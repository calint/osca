#define _XOPEN_SOURCE 500
#define _GNU_SOURCE /* To get defns of NI_MAXSERV and NI_MAXHOST */

#include "dc.h"
#include "graph.h"
#include "graphd.h"
#include "main-cfg.h"
#include "strb.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define APP_NAME "clonky system overview"

static struct dc *dc;
static struct graph *graph_cpu;
static struct graph *graph_mem;
static struct graphd *graph_net;

// prefix to battery directory
const char sys_cls_pwr[] = "/sys/class/power_supply/";

// quirk for different kernel names for battery charge indicator
const char *battery_energy_or_charge_prefix = "";

// auto_config_battery() copies the battery entry in '/sys/class/power_supply'I
static char sys_cls_pwr_bat[32];

// auto_config_network_traffic() copies the device name found in
// '/sys/class/net/' preferring wlan otherwise fallback on non loop-back device
static char graph_net_device[32];

static void get_sys_value_str_tolower(const char *path, char *value,
                                      const int size) {
  FILE *file = fopen(path, "r");
  if (!file) {
    *value = 0;
    return;
  }
  char fmt[32] = "";
  snprintf(fmt, sizeof(fmt), "%%%ds\\n",
           size - 1); // -1 to leave space for '\0'
  fscanf(file, fmt, value);
  fclose(file);
  char *p = value;
  while (*p) {
    *p = tolower(*p);
    p++;
  }
}

static int get_sys_value_int(const char *path) {
  FILE *file = fopen(path, "r");
  if (!file) {
    return 0;
  }
  int num = 0;
  fscanf(file, "%d", &num);
  fclose(file);
  return num;
}

static long long get_sys_value_long(const char *path) {
  FILE *file = fopen(path, "r");
  if (!file) {
    return 0;
  }
  long long num = 0;
  fscanf(file, "%lld\n", &num);
  fclose(file);
  return num;
}

static int sys_value_exists(const char *path) {
  const int result = access(path, F_OK);
  return result != -1 ? 1 : 0;
}

static void str_compact_spaces(char *str) {
  char *d = str;
  while (*str == ' ') {
    str++;
  }
  do {
    *d++ = *str;
    if (!*str) {
      return;
    }
    str++;
    const char is_space = *str == ' ';
    while (*str == ' ') {
      str++;
    }
    if (is_space && *str) {
      *d++ = ' ';
    }
  } while (1);
  *d = 0;
}

static void _rend_hr() { dc_draw_hr(dc); }

static void _rend_battery() {
  char buf[255] = "";
  const int nchars = snprintf(buf, sizeof(buf), "%s%s/%s_", sys_cls_pwr,
                              sys_cls_pwr_bat, battery_energy_or_charge_prefix);
  if (sizeof(buf) == nchars) {
    return;
  }
  const int maxlen = sizeof(buf) - nchars;
  char *p = buf + nchars;
  strncpy(p, "full", maxlen);
  const long long charge_full = get_sys_value_long(buf);
  strncpy(p, "now", maxlen);
  const long long charge_now = get_sys_value_long(buf);
  if (snprintf(buf, sizeof buf, "%s%s/status", sys_cls_pwr, sys_cls_pwr_bat) ==
      sizeof(buf)) {
    return;
  }
  get_sys_value_str_tolower(buf, buf, sizeof(buf));
  dc_newline(dc);
  char bbuf[1024];
  snprintf(bbuf, sizeof(bbuf), "battery %s  %lld/%lld mAh", buf,
           charge_now / 1000, charge_full / 1000);
  dc_draw_str(dc, bbuf);
  if (charge_full) {
    dc_draw_hr1(dc, WIDTH * charge_now / charge_full);
  }
}

static void _rend_cpu_load() {
  static int cpu_total_last = 0;
  static int cpu_usage_last = 0;

  FILE *file = fopen("/proc/stat", "r");
  if (!file)
    return;
  // user: normal processes executing in user mode
  // nice: niced processes executing in user mode
  // system: processes executing in kernel mode
  // idle: twiddling thumbs
  // iowait: waiting for I/O to complete
  // irq: servicing interrupts
  // softirq: servicing softirqs
  long long user, nice, system, idle, iowait, irq, softirq;
  char bbuf[1024] = "";
  fscanf(file, "%1023s %lld %lld %lld %lld %lld %lld %lld\n", bbuf, &user,
         &nice, &system, &idle, &iowait, &irq, &softirq);
  fclose(file);
  const long long total = user + nice + system + idle + iowait + irq + softirq;
  const long long usage = total - idle;
  const long long dtotal = total - cpu_total_last;
  cpu_total_last = total;
  const long long dusage = usage - cpu_usage_last;
  cpu_usage_last = usage;
  const long long usage_percent = dusage * 100 / dtotal;
  graph_add_value(graph_cpu, usage_percent);
  dc_inc_y(dc, DELTA_Y_HR);
  dc_inc_y(dc, DEFAULT_GRAPH_HEIGHT);
  graph_draw2(graph_cpu, dc, DEFAULT_GRAPH_HEIGHT, 100);
}

static void _rend_hello_clonky() {
  static long long unsigned counter;
  counter++;
  char bbuf[128];
  snprintf(bbuf, sizeof bbuf, "%llu hello%sclonky", counter,
           counter != 1 ? "s " : " ");
  dc_newline(dc);
  dc_draw_str(dc, bbuf);
}

static void _rend_mem_info() {
  FILE *file = fopen("/proc/meminfo", "r");
  if (!file) {
    return;
  }
  char name[32] = "";
  char unit[16] = "";
  long long mem_total = 0;
  long long mem_avail = 0;
  char bbuf[256] = "";
  fgets(bbuf, sizeof(bbuf), file); //	MemTotal:        1937372 kB
  sscanf(bbuf, "%63s %lld %31s", name, &mem_total, unit);
  fgets(bbuf, sizeof(bbuf), file); //	MemFree:           99120 kB
  fgets(bbuf, sizeof(bbuf), file); //	MemAvailable:     887512 kB
  fclose(file);

  sscanf(bbuf, "%31s %lld %15s", name, &mem_avail, unit);
  int proc = (mem_total - mem_avail) * 100 / mem_total;
  graph_add_value(graph_mem, proc);
  dc_inc_y(dc, DELTA_Y_HR);
  dc_inc_y(dc, DEFAULT_GRAPH_HEIGHT);
  graph_draw(graph_mem, dc, 2);
  if (mem_avail >> 10 != 0) {
    mem_avail >>= 10;
    mem_total >>= 10;
    strcpy(unit, "MB");
  }
  snprintf(bbuf, sizeof bbuf, "freemem %llu of %llu %s", mem_avail, mem_total,
           unit);
  dc_newline(dc);
  dc_draw_str(dc, bbuf);
}

static void _rend_net_traffic() {
  dc_inc_y(dc, DEFAULT_GRAPH_HEIGHT + DELTA_Y_HR);
  char bbuf[128] = "";
  snprintf(bbuf, sizeof(bbuf), "/sys/class/net/%s/statistics/tx_bytes",
           graph_net_device);
  long long wifi_tx = get_sys_value_long(bbuf);
  snprintf(bbuf, sizeof(bbuf), "/sys/class/net/%s/statistics/rx_bytes",
           graph_net_device);
  long long wifi_rx = get_sys_value_long(bbuf);
  graphd_add_value(graph_net, wifi_tx + wifi_rx);
  graphd_draw(graph_net, dc, DEFAULT_GRAPH_HEIGHT, NET_GRAPH_MAX);
}

static void pl(const char *str) {
  dc_newline(dc);
  dc_draw_str(dc, str);
}

static void _rend_cheetsheet() {
  static char *keysheet[] = {
      "ĸey", "+c               console", "+f                 files",
      "+e                editor", "+m                 media",
      "+v                 mixer", "+i              internet",
      "+x                sticky", "+q              binaries",
      //	"+prtsc          snapshot",
      "+p              snapshot", "", "đesktop", "+up                   up",
      "+down               down", "+shift+up        move-up",
      "+shift+down    move-down", "", "window", "+esc               close",
      "+b                  bump", "+s                center",
      "+w                 wider", "+W               thinner",
      "+r                resize", "+3            fullscreen",
      "+4           full height", "+5            full width",
      "+6   i-am-bored-surprise", "...                  ...", NULL};

  char **str_ptr = keysheet;
  while (*str_ptr) {
    dc_newline(dc);
    dc_draw_str(dc, *str_ptr);
    str_ptr++;
  }
}

static void _rend_df() {
  FILE *f = popen("df -h 2>/dev/null", "r");
  if (!f) {
    return;
  }
  char bbuf[256] = "";
  while (1) {
    if (!fgets(bbuf, sizeof(bbuf), f)) {
      break;
    }
    str_compact_spaces(bbuf);
    if (bbuf[0] != '/') {
      continue;
    }
    pl(bbuf);
  }
  pclose(f);
}

static void _rend_io_stat() {
  static long long last_kb_read = 0;
  static long long last_kb_written = 0;

  FILE *file = popen("iostat -d", "r");
  if (!file) {
    return;
  }
  //	Linux 3.11.0-14-generic (vaio) 	03/12/2014 	_x86_64_	(2 CPU)
  //
  //	Device:            tps    kB_read/s    kB_wrtn/s    kB_read    kB_wrtn
  //	sda               7.89        25.40        80.46     914108    2896281
  char bbuf[512] = "";
  fgets(bbuf, sizeof(bbuf), file);
  fgets(bbuf, sizeof(bbuf), file);
  fgets(bbuf, sizeof(bbuf), file);
  float tps = 0;
  float kb_read_s = 0;
  float kb_written_s = 0;
  long long kb_read = 0;
  long long kb_written = 0;
  char dev[64] = "";
  fscanf(file, "%63s %f %f %f %lld %lld", dev, &tps, &kb_read_s, &kb_written_s,
         &kb_read, &kb_written);
  pclose(file);
  const char *unit = "kB";
  snprintf(bbuf, sizeof(bbuf), "read %lld %s wrote %lld %s",
           kb_read - last_kb_read, unit, kb_written - last_kb_written, unit);
  pl(bbuf);
  last_kb_read = kb_read;
  last_kb_written = kb_written;
}

static void _rend_dmsg() {
  FILE *file = popen("journalctl --lines=15 --no-pager", "r");
  if (!file) {
    return;
  }
  char bbuf[1024];
  while (1) {
    if (!fgets(bbuf, sizeof(bbuf), file)) {
      break;
    }
    pl(bbuf);
  }
  pclose(file);
}

static void _rend_acpi() {
  FILE *file =
      popen("acpi -V | grep -vi 'no state information available'", "r");
  if (!file) {
    return;
  }
  while (1) {
    char bbuf[512];
    if (!fgets(bbuf, sizeof(bbuf), file)) {
      break;
    }
    for (char *p = bbuf; *p; ++p) {
      *p = tolower(*p);
    }
    pl(bbuf);
  }
  pclose(file);
}

inline static void _rend_date_time() {
  const time_t t = time(NULL);
  const struct tm *lt = localtime(&t); //? free?
  strb sb;
  strb_init(&sb);
  if (strb_p(&sb, asctime(lt))) {
    return;
  }
  dc_newline(dc);
  dc_draw_str(dc, sb.chars);
}

static void _rend_cpu_throttles() {
  FILE *file = fopen("/sys/devices/system/cpu/present", "r");
  if (!file) {
    return;
  }
  int min = 0;
  int max = 0;
  fscanf(file, "%d-%d", &min, &max);
  fclose(file);

  strb sb;
  strb_init(&sb);
  if (strb_p(&sb, "throttle "))
    return;

  for (int i = min; i <= max; i++) {
    char bbuf[512];
    snprintf(bbuf, sizeof bbuf,
             "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", i);
    const long long max_freq = get_sys_value_long(bbuf);
    snprintf(bbuf, sizeof bbuf,
             "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
    const long long cur_freq = get_sys_value_long(bbuf);
    strb_p(&sb, " ");
    if (max_freq) {
      // if available render percent of max frequency
      const long long proc = (cur_freq * 100) / max_freq;
      strb_fmt_long(&sb, proc);
      strb_p(&sb, "%");
    } else {
      // max frequency not available
      strb_p(&sb, "n/a");
    }
  }
  pl(sb.chars);
}

static void _rend_swaps() {
  FILE *file = fopen("/proc/swaps", "r");
  if (!file)
    return;
  // Filename         Type       Size     Used   Priority
  // /dev/mmcblk0p3   partition  2096124  16568  s-1
  char bbuf[1024];
  fgets(bbuf, sizeof bbuf, file);
  char dev[64] = "";
  char type[32] = "";
  long long size = 0;
  long long used = 0;
  if (!fscanf(file, "%63s %31s %lld %lld", dev, type, &size, &used)) {
    return;
  }
  fclose(file);

  strb sb;
  strb_init(&sb);
  if (strb_p(&sb, "swapped ")) {
    return;
  }
  if (strb_fmt_bytes(&sb, used << 10)) {
    return;
  }
  pl(sb.chars);
}

static void auto_config_battery() {
  DIR *dir = opendir("/sys/class/power_supply");
  if (!dir) {
    puts("[!] battery: cannot open find dir '/sys/class/power_supply'");
    return;
  }
  struct dirent *entry;
  *sys_cls_pwr_bat = 0;
  while ((entry = readdir(dir))) {
    if (entry->d_name[0] == '.') {
      // skip hidden files
      continue;
    }
    // find out if type is battery
    char buf[512];
    if (snprintf(buf, sizeof(buf), "/sys/class/power_supply/%s/type",
                 entry->d_name) == sizeof(buf)) {
      printf("%s:%d - buffer probably overrun\n", __FILE__, __LINE__);
    }
    get_sys_value_str_tolower(buf, buf, sizeof(buf));
    if (strcmp(buf, "battery")) {
      continue;
    }
    // found 'battery'
    strncpy(sys_cls_pwr_bat, entry->d_name, sizeof(sys_cls_pwr_bat));

    //? quirk if it energy_full_design  charge_full_design
    if (snprintf(buf, sizeof(buf), "/sys/class/power_supply/%s/energy_now",
                 sys_cls_pwr_bat) == sizeof(buf)) {
      return;
    }
    if (sys_value_exists(buf)) {
      battery_energy_or_charge_prefix = "energy";
      break;
    }
    if (snprintf(buf, sizeof(buf), "/sys/class/power_supply/%s/charge_now",
                 sys_cls_pwr_bat) == sizeof buf) {
      return;
    }
    if (sys_value_exists(buf)) {
      battery_energy_or_charge_prefix = "charge";
      break;
    }
    printf("%s %d - energy or charge not resolved\n", __FILE__, __LINE__);
    break;
  }
  closedir(dir);
  if (!sys_cls_pwr_bat[0]) {
    puts("[!] no battery found in /sys/class/power_supply");
  }
  printf("· battery: ");
  puts(sys_cls_pwr_bat);
  return;
}

static int is_wlan_device(const char *sys_cls_net_wlan) {
  // build string to file '/sys/class/net/XXX/wireless
  strb sb;
  strb_init(&sb);
  if (strb_p(&sb, "/sys/class/net/")) {
    return 0;
  }
  if (strb_p(&sb, sys_cls_net_wlan)) {
    return 0;
  }
  if (strb_p(&sb, "/wireless")) {
    return 0;
  }
  struct stat st;
  if (stat(sb.chars, &st)) {
    return 0;
  }
  return 1;
}

static void auto_config_network_traffic() {
  DIR *dir = opendir("/sys/class/net");
  if (!dir) {
    puts("[!] wifi: cannot open find dir /sys/class/net");
    return;
  }
  struct dirent *entry;
  graph_net_device[0] = '\0';
  while ((entry = readdir(dir))) {
    if (entry->d_name[0] == '.') {
      // ignore hidden files
      continue;
    }
    if (is_wlan_device(entry->d_name)) {
      // found wlan device (preferred)
      strncpy(graph_net_device, entry->d_name, sizeof(graph_net_device));
      break;
    }
    if (!strcmp("lo", entry->d_name)) {
      // skip loopback
      continue;
    }
    // network device (not preferred)
    strncpy(graph_net_device, entry->d_name, sizeof(graph_net_device));
  }
  closedir(dir);
  if (!graph_net_device[0]) {
    puts("[!] no network device found in /sys/class/net");
  }
  printf("· network device: ");
  puts(graph_net_device);
  return;
}

static void auto_config() {
  auto_config_battery();
  auto_config_network_traffic();
}

static struct ifc {
  /*ref*/ const char *name;
  unsigned long long rx_bytes, tx_bytes;
  /*own*/ char *hostname;
  /*ref*/ struct ifc *next;
} *ifcs;

// static struct ifc*ifcs;
static void ifcs_delete() {
  struct ifc *ifc = ifcs;
  while (ifc != NULL) {
    free(ifc->hostname);
    struct ifc *nxt = ifc->next;
    free(ifc);
    ifc = nxt;
  }
  ifcs = NULL;
}

static void ifcs_for_each(int f(struct ifc *)) {
  struct ifc *ifc = ifcs;
  while (ifc != NULL) {
    const int ret = f(ifc);
    if (ret) {
      break;
    }
    ifc = ifc->next;
  }
}

static void ifcs_add_first(/*takes*/ struct ifc *ifc) {
  if (ifcs) {
    ifc->next = ifcs;
    ifcs = ifc;
    return;
  }
  ifcs = ifc;
  ifc->next = NULL;
}

static struct ifc *ifcs_get_by_name(/*refs*/ const char *name) {
  struct ifc *ifc = ifcs;
  while (ifc != NULL) {
    if (!strncmp(ifc->name, name, NI_MAXHOST)) {
      return ifc;
    }
    ifc = ifc->next;
  }
  ifc = (struct ifc *)calloc(1, sizeof(struct ifc));
  ifc->name = name;
  ifcs_add_first(/*give*/ ifc);
  return ifc;
}

int _rend_net_callback(struct ifc *ifc) {
  char buf[1024];
  snprintf(buf, sizeof(buf), "%s  %s  %llu/%llu MB", ifc->name,
           ifc->hostname ? "up" : "down", ifc->tx_bytes >> 20,
           ifc->rx_bytes >> 10);
  dc_newline(dc);
  dc_draw_str(dc, buf);
  return 0;
}

int _rend_net() {
  struct ifaddrs *ifas, *ifa;
  if (getifaddrs(&ifas) == -1) {
    perror("getifaddrs");
    return -1;
  }
  ifa = ifas;
  int ret = 0;
  for (int i = 0; ifa != NULL; ifa = ifa->ifa_next, i++) {
    if (ifa->ifa_addr == NULL) {
      continue;
    }
    const int family = ifa->ifa_addr->sa_family;
    const char *name = ifa->ifa_name;
    struct ifc *ifc = ifcs_get_by_name(name);
    if (family == AF_PACKET && ifa->ifa_data != NULL) {
      struct rtnl_link_stats *stats = ifa->ifa_data;
      ifc->rx_bytes = stats->rx_bytes;
      ifc->tx_bytes = stats->tx_bytes;
    }
    //		if(family==AF_INET||family==AF_INET6){
    if (family == AF_INET) {
      ifc->hostname = malloc(NI_MAXHOST);
      const int name_info =
          getnameinfo(ifa->ifa_addr,
                      family == AF_INET ? sizeof(struct sockaddr_in)
                                        : sizeof(struct sockaddr_in6),
                      ifc->hostname, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
      if (name_info != 0) {
        //				printf("%s",gai_strerror(s));
        //				ret=-2;
        //				goto cleanup;
        continue;
      }
    }
  }
  ifcs_for_each(_rend_net_callback);
  // cleanup:
  freeifaddrs(ifas);
  ifcs_delete();
  return ret;
}

static void draw() {
  dc_set_y(dc, TOP_Y);
  dc_clear(dc);
  _rend_date_time();
  _rend_cpu_load();
  _rend_hello_clonky();
  _rend_mem_info();
  _rend_swaps();
  _rend_net_traffic();
  _rend_net();
  _rend_hr();
  _rend_io_stat();
  _rend_df();
  _rend_hr();
  _rend_cpu_throttles();
  _rend_battery();
  _rend_hr();
  _rend_acpi(dc);
  _rend_hr();
  _rend_dmsg();
  _rend_hr();
  _rend_hr();
  _rend_cheetsheet();
  _rend_hr();
  _rend_hr();
  dc_flush(dc);
}

static void signal_exit(int i) {
  puts("exiting");
  dc_del(dc);
  if (graph_cpu) {
    graph_del(graph_cpu);
  }
  if (graph_mem) {
    graph_del(graph_mem);
  }
  if (graph_net) {
    graphd_del(graph_net);
  }
  signal(SIGINT, SIG_DFL);
  kill(getpid(), SIGINT);
  exit(i);
}

int main() {
  signal(SIGINT, signal_exit);

  puts(APP_NAME);

  if (!(dc = dc_new())) {
    exit(1);
  }
  dc_set_width(dc, WIDTH);
  if (ALIGN == 1) {
    dc_set_left_x(dc, dc_get_screen_width(dc) - WIDTH);
  }
  graph_cpu = graph_new(WIDTH);
  graph_mem = graph_new(WIDTH);
  graph_net = graphd_new(WIDTH);

  auto_config();

  while (1) {
    sleep(1);
    draw();
  }
}

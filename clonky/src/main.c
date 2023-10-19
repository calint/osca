#define _GNU_SOURCE // To get defines of NI_MAXSERV and NI_MAXHOST

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

// device context (renderer)
static struct dc *dc;

// cpu usage graph
static struct graph *graph_cpu;

// memory usage graph
static struct graph *graph_mem;

// network traffic graph
static struct graphd *graph_net;

// prefix to battery status directory
static const char power_supply_path_prefix[] = "/sys/class/power_supply/";

// quirk for different names for battery charge indicator
static const char *battery_energy_or_charge_prefix = "";

// auto_config_battery() copies the battery entry in '/sys/class/power_supply/'I
static char battery_name[32] = "";

// auto_config_network_traffic() copies the device name found in
// '/sys/class/net/' preferring wlan otherwise fallback on non loop-back device
static char net_device[32] = "";

// network interfaces
static struct netifc {
  // name as listed in '/sys/class/net/'
  char name[NETIFC_NAME_SIZE];
  // bytes received at previous check
  long long rx_bytes_prv;
  // bytes transmitted at previous check
  long long tx_bytes_prv;
} netifcs[NETIFC_ARRAY_SIZE];

// used network interfaces
static unsigned netifcs_len;

static void get_sys_value_str_tolower(const char *path, char *value,
                                      const int size) {
  FILE *file = fopen(path, "r");
  if (!file) {
    value[0] = '\0';
    return;
  }
  char fmt[32] = "";
  // -1 to leave space for '\0'
  snprintf(fmt, sizeof(fmt), "%%%ds\\n", size - 1);
  fscanf(file, fmt, value);
  fclose(file);
  char *p = value;
  while (*p) {
    *p = (char)tolower(*p);
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

static int sys_value_exists(const char *path) { return !access(path, F_OK); }

static void str_compact_spaces(char *str) {
  // "   a  b c  "
  char *dst = str;
  while (*str == ' ') {
    str++;
  }
  // "a  b c  "
  while (1) {
    *dst++ = *str;
    if (!*str) {
      // "a b c "
      return;
    }
    str++;
    int is_spc = 0;
    while (*str == ' ') {
      str++;
      is_spc++;
    }
    if (is_spc) {
      *dst++ = ' ';
    }
  }
}

static void render_hr(void) { dc_draw_hr(dc); }

static void render_battery(void) {
  char buf[128] = "";
  const int nchars =
      snprintf(buf, sizeof(buf), "%s%s/%s_", power_supply_path_prefix,
               battery_name, battery_energy_or_charge_prefix);
  if (nchars < 0 || (size_t)nchars >= sizeof(buf)) {
    return; // truncated
  }
  const size_t maxlen = sizeof(buf) - (size_t)nchars;
  char *p = buf + nchars;
  strncpy(p, "full", maxlen);
  const long long charge_full = get_sys_value_long(buf);
  strncpy(p, "now", maxlen);
  const long long charge_now = get_sys_value_long(buf);
  snprintf(buf, sizeof(buf), "%s%s/status", power_supply_path_prefix,
           battery_name);
  get_sys_value_str_tolower(buf, buf, sizeof(buf));
  dc_newline(dc);
  char bbuf[1024];
  snprintf(bbuf, sizeof(bbuf), "battery %s  %lld/%lld mAh", buf,
           charge_now / 1000, charge_full / 1000);
  dc_draw_str(dc, bbuf);
  if (charge_full) {
    dc_draw_hr1(dc, (int)(WIDTH * charge_now / charge_full));
  }
}

static void render_cpu_load(void) {
  static long long cpu_total_last = 0;
  static long long cpu_usage_last = 0;

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

static void render_hello_clonky(void) {
  static long long unsigned counter = 0;
  counter++;
  char bbuf[128];
  snprintf(bbuf, sizeof bbuf, "%llu hello%sclonky", counter,
           counter != 1 ? "s " : " ");
  dc_newline(dc);
  dc_draw_str(dc, bbuf);
}

static void render_mem_info(void) {
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
  sscanf(bbuf, "%31s %lld %15s", name, &mem_total, unit);
  fgets(bbuf, sizeof(bbuf), file); //	MemFree:           99120 kB
  fgets(bbuf, sizeof(bbuf), file); //	MemAvailable:     887512 kB
  fclose(file);

  sscanf(bbuf, "%31s %lld %15s", name, &mem_avail, unit);
  int proc = (int)((mem_total - mem_avail) * 100 / mem_total);
  graph_add_value(graph_mem, proc);
  dc_inc_y(dc, DELTA_Y_HR);
  dc_inc_y(dc, DEFAULT_GRAPH_HEIGHT);
  graph_draw(graph_mem, dc, 2);
  if (mem_avail >> 10 != 0) {
    mem_avail >>= 10;
    mem_total >>= 10;
    strcpy(unit, "MB");
  }
  snprintf(bbuf, sizeof(bbuf), "freemem %llu of %llu %s", mem_avail, mem_total,
           unit);
  dc_newline(dc);
  dc_draw_str(dc, bbuf);
}

static void render_net_traffic(void) {
  dc_inc_y(dc, DEFAULT_GRAPH_HEIGHT + DELTA_Y_HR);
  char bbuf[128] = "";
  snprintf(bbuf, sizeof(bbuf), "/sys/class/net/%s/statistics/tx_bytes",
           net_device);
  long long wifi_tx = get_sys_value_long(bbuf);
  snprintf(bbuf, sizeof(bbuf), "/sys/class/net/%s/statistics/rx_bytes",
           net_device);
  long long wifi_rx = get_sys_value_long(bbuf);
  graphd_add_value(graph_net, wifi_tx + wifi_rx);
  graphd_draw(graph_net, dc, DEFAULT_GRAPH_HEIGHT, NET_GRAPH_MAX);
}

static void pl(const char *str) {
  dc_newline(dc);
  dc_draw_str(dc, str);
}

static void render_cheetsheet(void) {
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

static void render_df(void) {
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

static void render_io_stat(void) {
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

static void render_dmsg(void) {
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

static void render_acpi(void) {
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
      *p = (char)tolower(*p);
    }
    pl(bbuf);
  }
  pclose(file);
}

static void render_date_time(void) {
  const time_t t = time(NULL);
  const struct tm *lt = localtime(&t);
  strb sb;
  strb_init(&sb);
  if (strb_p(&sb, asctime(lt))) {
    return;
  }
  // delete the '\n' that asctime writes at the end of the string
  strb_back(&sb);
  dc_newline(dc);
  dc_draw_str(dc, sb.chars);
}

static void render_cpu_throttles(void) {
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
  if (strb_p(&sb, "throttle ")) {
    return;
  }

  for (int i = min; i <= max; i++) {
    char bbuf[128];
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
      strb_p_long(&sb, proc);
      strb_p(&sb, "%");
    } else {
      // max frequency not available
      strb_p(&sb, "n/a");
    }
  }
  pl(sb.chars);
}

static void render_swaps(void) {
  FILE *file = fopen("/proc/swaps", "r");
  if (!file) {
    return;
  }
  // Filename         Type       Size     Used   Priority
  // /dev/mmcblk0p3   partition  2096124  16568  s-1
  char bbuf[1024];
  fgets(bbuf, sizeof bbuf, file);
  char dev[64] = "";
  char type[32] = "";
  long long size = 0;
  long long used = 0;
  if (!fscanf(file, "%63s %31s %lld %lld", dev, type, &size, &used)) {
    fclose(file);
    return;
  }
  fclose(file);

  strb sb;
  strb_init(&sb);
  if (strb_p(&sb, "swapped ")) {
    return;
  }
  if (strb_p_nbytes(&sb, used << 10)) {
    return;
  }
  pl(sb.chars);
}

static void auto_config_battery(void) {
  DIR *dir = opendir("/sys/class/power_supply");
  if (!dir) {
    puts("[!] battery: cannot open find dir '/sys/class/power_supply'");
    return;
  }
  struct dirent *entry;
  *battery_name = 0;
  while ((entry = readdir(dir))) {
    if (entry->d_name[0] == '.') {
      // skip hidden files
      continue;
    }
    // find out if type is battery
    char buf[128];
    snprintf(buf, sizeof(buf), "/sys/class/power_supply/%.*s/type", 64,
             entry->d_name);
    get_sys_value_str_tolower(buf, buf, sizeof(buf));
    if (strcmp(buf, "battery")) {
      continue;
    }
    // found 'battery'
    strncpy(battery_name, entry->d_name, sizeof(battery_name));

    //? quirk if it energy_full_design  charge_full_design
    snprintf(buf, sizeof(buf), "/sys/class/power_supply/%s/energy_now",
             battery_name);
    if (sys_value_exists(buf)) {
      battery_energy_or_charge_prefix = "energy";
      break;
    }
    snprintf(buf, sizeof(buf), "/sys/class/power_supply/%s/charge_now",
             battery_name);
    if (sys_value_exists(buf)) {
      battery_energy_or_charge_prefix = "charge";
      break;
    }
    printf("%s %d - energy or charge not resolved\n", __FILE__, __LINE__);
    break;
  }
  closedir(dir);
  if (!battery_name[0]) {
    puts("[!] no battery found in /sys/class/power_supply");
  }
  printf("· battery: ");
  puts(battery_name);
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
  return !stat(sb.chars, &st);
}

static void auto_config_network_traffic(void) {
  DIR *dir = opendir("/sys/class/net");
  if (!dir) {
    puts("[!] wifi: cannot open find dir /sys/class/net");
    return;
  }
  struct dirent *entry;
  net_device[0] = '\0';
  while ((entry = readdir(dir))) {
    if (entry->d_name[0] == '.') {
      // ignore hidden files
      continue;
    }
    if (is_wlan_device(entry->d_name)) {
      // found wlan device (preferred)
      strncpy(net_device, entry->d_name, sizeof(net_device));
      break;
    }
    if (!strcmp("lo", entry->d_name)) {
      // skip loopback
      continue;
    }
    // network device (not preferred)
    strncpy(net_device, entry->d_name, sizeof(net_device));
  }
  closedir(dir);
  if (!net_device[0]) {
    puts("[!] no network device found in /sys/class/net");
  }
  printf("· graph network interface: ");
  puts(net_device);
  return;
}

static void auto_config(void) {
  auto_config_battery();
  auto_config_network_traffic();
}

static struct netifc *netifcs_get_by_name_or_create(const char *name) {
  for (unsigned i = 0; i < NETIFC_ARRAY_SIZE; i++) {
    struct netifc *ni = &netifcs[i];
    if (!strncmp(ni->name, name, sizeof(ni->name))) {
      return &netifcs[i];
    }
  }
  if (netifcs_len >= NETIFC_ARRAY_SIZE) {
    return NULL;
  }
  printf("· network interface: %s\n", name);
  struct netifc *ni = &netifcs[netifcs_len];
  netifcs_len++;
  strncpy(ni->name, name, sizeof(ni->name));
  return ni;
}

static void render_net_interfaces(void) {
  DIR *dir = opendir("/sys/class/net/");
  if (!dir) {
    return;
  }
  char operstate[32] = "";
  char buf[256] = "";
  for (struct dirent *entry = readdir(dir); entry; entry = readdir(dir)) {
    if (entry->d_name[0] == '.') {
      continue; // ignore '.' and '..'
    }
    snprintf(buf, sizeof(buf), "/sys/class/net/%.*s/operstate", 32,
             entry->d_name);
    get_sys_value_str_tolower(buf, operstate, sizeof(operstate));
    snprintf(buf, sizeof(buf), "/sys/class/net/%.*s/statistics/tx_bytes", 32,
             entry->d_name);
    long long tx_bytes = get_sys_value_long(buf);
    snprintf(buf, sizeof(buf), "/sys/class/net/%.*s/statistics/rx_bytes", 32,
             entry->d_name);
    long long rx_bytes = get_sys_value_long(buf);

    struct netifc *ifc = netifcs_get_by_name_or_create(entry->d_name);
    if (!ifc) {
      return;
    }
    long long delta_tx_bytes =
        tx_bytes - (ifc->tx_bytes_prv ? ifc->tx_bytes_prv : tx_bytes);
    long long delta_rx_bytes =
        rx_bytes - (ifc->rx_bytes_prv ? ifc->rx_bytes_prv : rx_bytes);

    ifc->tx_bytes_prv = tx_bytes;
    ifc->rx_bytes_prv = rx_bytes;

    const char *rx_scale = "B/s";
    if (delta_rx_bytes >> 20) {
      delta_rx_bytes >>= 20;
      rx_scale = "MB/s";
    } else if (delta_rx_bytes >> 10) {
      delta_rx_bytes >>= 10;
      rx_scale = "KB/s";
    }

    const char *tx_scale = "B/s";
    if (delta_tx_bytes >> 20) {
      delta_tx_bytes >>= 20;
      tx_scale = "MB/s";
    } else if (delta_tx_bytes >> 10) {
      delta_tx_bytes >>= 10;
      tx_scale = "KB/s";
    }

    if (!strcmp(entry->d_name, "lo")) {
      operstate[0] = '\0'; // empty string for 'lo'
    }
    snprintf(buf, sizeof(buf), "%.*s %s ↓ %lld %s ↑ %lld %s", 16, entry->d_name,
             operstate, delta_rx_bytes, rx_scale, delta_tx_bytes, tx_scale);
    // puts(buf);
    pl(buf);
  }
  closedir(dir);
}

static void signal_exit(int i) {
  puts("\nexiting");
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
  exit(i);
}

static void draw(void) {
  dc_set_y(dc, TOP_Y);
  dc_clear(dc);
  render_date_time();
  render_cpu_load();
  render_hello_clonky();
  render_mem_info();
  render_swaps();
  render_net_traffic();
  render_net_interfaces();
  render_hr();
  render_io_stat();
  render_df();
  render_hr();
  render_cpu_throttles();
  render_battery();
  render_hr();
  render_acpi();
  render_hr();
  render_dmsg();
  render_hr();
  render_hr();
  render_cheetsheet();
  render_hr();
  render_hr();
  dc_flush(dc);
}

int main(int argc, char *argv[]) {
  signal(SIGINT, signal_exit);

  puts("clonky system overview");
  while (argc--) {
    puts(*argv++);
  }

  if (!(dc = dc_new())) {
    exit(1);
  }

  dc_set_width(dc, WIDTH);

  if (ALIGN == 1) {
    // align right
    dc_set_left_x(dc, (int)dc_get_screen_width(dc) - WIDTH);
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

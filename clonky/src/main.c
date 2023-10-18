#define _XOPEN_SOURCE 500
#include "dc.h"
#include "graph.h"
#include "graphd.h"
#include "main-cfg.h"
#include "net.h"
#include "strb.h"
#include <ctype.h>
#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define APP_NAME "clonky system overview"

static struct dc *dc;
static struct graph *graphcpu;
static struct graph *graphmem;
static struct graphd *graphwifi;

static char sys_cls_pwr_bat[64];
static char sys_cls_net_wlan[64];

static void get_sys_value_str_tolower(const char *path, char *value,
                                      const int size) {
  FILE *file = fopen(path, "r");
  if (!file) {
    *value = 0;
    return;
  }
  char fmt[32];
  snprintf(fmt, sizeof(fmt), "%%%ds\\n", size);
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
static void str_compact_spaces(char *s) {
  char *d = s;
  if (!*s) {
    *d = 0;
    return;
  }
  while (*s == ' ') {
    s++;
  }
  do {
    *d++ = *s;
    if (!*s) {
      return;
    }
    s++;
    const char is_space = *s == ' ';
    while (*s == ' ') {
      s++;
    }
    if (is_space && *s) {
      *d++ = ' ';
    }
  } while (1);
  *d = 0;
}

static void _rend_hr() { dc_draw_hr(dc); }

const char sys_cls_pwr[] = "/sys/class/power_supply/";
const char *battery_energy_or_charge_prefix = "";

static void _rend_battery() {
  char buf[255] = "";
  const int nchars = snprintf(buf, sizeof(buf), "%s%s/%s_", sys_cls_pwr,
                              sys_cls_pwr_bat, battery_energy_or_charge_prefix);
  if (sizeof(buf) == nchars) {
    printf("%s %d: probably truncated path: %s\n", __FILE__, __LINE__, buf);
  }
  const int maxlen = sizeof(buf) - nchars;
  char *p = buf + nchars; //? snprintf
  strncpy(p, "full", maxlen);
  const long long charge_full = get_sys_value_long(buf);
  strncpy(p, "now", maxlen);
  const long long charge_now = get_sys_value_long(buf);
  if (snprintf(buf, sizeof buf, "%s%s/status", sys_cls_pwr, sys_cls_pwr_bat) ==
      sizeof(buf)) {
    printf("%s %d: probably truncated path: %s\n", __FILE__, __LINE__, buf);
  }
  get_sys_value_str_tolower(buf, buf, sizeof(buf));
  dc_newline(dc);
  char bbuf[1024];
  snprintf(bbuf, sizeof(bbuf), "battery %s  %lld/%lld mAh", buf,
           charge_now / 1000, charge_full / 1000);
  dc_draw_str(dc, bbuf);
  if (charge_full)
    dc_draw_hr1(dc, WIDTH * charge_now / charge_full);
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
  int user, nice, system, idle, iowait, irq, softirq;
  char bbuf[1024];
  fscanf(file, "%1024s %d %d %d %d %d %d %d\n", bbuf, &user, &nice, &system,
         &idle, &iowait, &irq, &softirq);
  fclose(file);
  const int total = (user + nice + system + idle + iowait + irq + softirq);
  const int usage = total - idle;
  const long long dtotal = total - cpu_total_last;
  cpu_total_last = total;
  const int dusage = usage - cpu_usage_last;
  cpu_usage_last = usage;
  const int usagepercent = dusage * 100 / dtotal;
  graph_add_value(graphcpu, usagepercent);
  dc_inc_y(dc, DELTA_Y_HR);
  dc_inc_y(dc, DEFAULT_GRAPH_HEIGHT);
  graph_draw2(graphcpu, dc, DEFAULT_GRAPH_HEIGHT, 100);
}

static void _rend_hello_clonky() {
  static long long unsigned counter;
  counter++;
  char bbuf[1024];
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
  char name[64] = "", unit[32] = "";
  long long mem_total = 0, mem_avail = 0;
  char bbuf[1024];
  fgets(bbuf, sizeof(bbuf), file); //	MemTotal:        1937372 kB
  sscanf(bbuf, "%64s %lld %32s", name, &mem_total, unit);
  fgets(bbuf, sizeof(bbuf), file); //	MemFree:           99120 kB
  fgets(bbuf, sizeof(bbuf), file); //	MemAvailable:     887512 kB
  fclose(file);

  sscanf(bbuf, "%64s %lld %32s", name, &mem_avail, unit);
  int proc = (mem_total - mem_avail) * 100 / mem_total;
  graph_add_value(graphmem, proc);
  dc_inc_y(dc, DELTA_Y_HR);
  dc_inc_y(dc, DEFAULT_GRAPH_HEIGHT);
  graph_draw(graphmem, dc, 2);
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

static void _rend_wifi_traffic() {
  dc_inc_y(dc, DEFAULT_GRAPH_HEIGHT + DELTA_Y_HR);
  char bbuf[1024];
  snprintf(bbuf, sizeof bbuf, "/sys/class/net/%s/statistics/tx_bytes",
           sys_cls_net_wlan);
  long long wifi_tx = get_sys_value_long(bbuf);
  snprintf(bbuf, sizeof bbuf, "/sys/class/net/%s/statistics/rx_bytes",
           sys_cls_net_wlan);
  long long wifi_rx = get_sys_value_long(bbuf);
  graphd_add_value(graphwifi, wifi_tx + wifi_rx);
  graphd_draw(graphwifi, dc, DEFAULT_GRAPH_HEIGHT, WIFI_GRAPH_MAX);
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

  char **strptr = keysheet;
  while (*strptr) {
    dc_newline(dc);
    dc_draw_str(dc, *strptr);
    strptr++;
  }
}

static void _rend_df() {
  FILE *f = popen("df -h 2>/dev/null", "r");
  if (!f) {
    return;
  }
  char bbuf[1024] = "";
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

  FILE *f = popen("iostat -d", "r");
  if (!f) {
    return;
  }
  //	Linux 3.11.0-14-generic (vaio) 	03/12/2014 	_x86_64_	(2 CPU)
  //
  //	Device:            tps    kB_read/s    kB_wrtn/s    kB_read    kB_wrtn
  //	sda               7.89        25.40        80.46     914108    2896281
  char bbuf[1024];
  fgets(bbuf, sizeof(bbuf), f);
  fgets(bbuf, sizeof(bbuf), f);
  fgets(bbuf, sizeof(bbuf), f);
  float tps = 0, kb_read_s = 0, kb_written_s = 0;
  long long kb_read = 0, kb_written = 0;
  char dev[64];
  fscanf(f, "%64s %f %f %f %lld %lld", dev, &tps, &kb_read_s, &kb_written_s,
         &kb_read, &kb_written);
  pclose(f);
  const char *unit = "kB";
  snprintf(bbuf, sizeof(bbuf), "read %lld %s wrote %lld %s",
           kb_read - last_kb_read, unit, kb_written - last_kb_written, unit);
  pl(bbuf);
  last_kb_read = kb_read;
  last_kb_written = kb_written;
}

static void _rend_dmsg() {
  FILE *f = popen("journalctl --lines=15 --no-pager", "r");
  if (!f) {
    return;
  }
  char bbuf[1024];
  while (1) {
    if (!fgets(bbuf, sizeof(bbuf), f)) {
      break;
    }
    pl(bbuf);
  }
  pclose(f);
}

static void _rend_acpi() {
  FILE *f = popen("acpi -V | grep -vi 'no state information available'", "r");
  if (!f) {
    return;
  }
  while (1) {
    char bbuf[1024];
    if (!fgets(bbuf, sizeof bbuf, f)) {
      break;
    }
    for (char *p = bbuf; *p; ++p) {
      *p = tolower(*p);
    }
    pl(bbuf);
  }
  pclose(f);
}

inline static void _rend_datetime() {
  const time_t t = time(NULL);
  const struct tm *lt = localtime(&t); //? free?
  strb sb;
  strb_init(&sb);
  if (strb_p(&sb, asctime(lt)))
    return;
  dc_newline(dc);
  dc_draw_str(dc, sb.chars);
}

static void _rend_cpu_throttles() {
  FILE *f = fopen("/sys/devices/system/cpu/present", "r");
  if (!f)
    return;
  int min, max;
  fscanf(f, "%d-%d", &min, &max);
  fclose(f);

  strb sb;
  strb_init(&sb);
  if (strb_p(&sb, "throttle "))
    return;

  for (int n = min; n <= max; n++) {
    char bbuf[512];
    snprintf(bbuf, sizeof bbuf,
             "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq", n);
    const long long max_freq = get_sys_value_long(bbuf);
    snprintf(bbuf, sizeof bbuf,
             "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", n);
    const long long cur_freq = get_sys_value_long(bbuf);
    strb_p(&sb, " ");
    const long long proc = max_freq == 0 ? 0 : (cur_freq * 100) / max_freq;
    strb_fmt_long(&sb, proc);
    strb_p(&sb, "%");
    //		printf("%d  %lld    %lld   %lld\n",n,proc,cur_freq,max_freq);
  }
  pl(sb.chars);
}

static void _rend_swaps() {
  FILE *f = fopen("/proc/swaps", "r");
  if (!f)
    return;
  //	Filename				Type		Size	Used
  // Priority 	/dev/mmcblk0p3                          partition	2096124
  // 16568 -1
  char bbuf[1024];
  fgets(bbuf, sizeof bbuf, f);
  char dev[64], type[32];
  long long size = 0, used = 0;
  if (!fscanf(f, "%64s %32s %lld %lld", dev, type, &size, &used))
    return;
  fclose(f);

  strb sb;
  strb_init(&sb);
  if (strb_p(&sb, "swapped "))
    return;
  if (strb_fmt_bytes(&sb, used << 10))
    return;
  pl(sb.chars);
}

static void auto_config_bat() {
  DIR *dp = opendir("/sys/class/power_supply");
  if (!dp) {
    puts("[!] battery: cannot open find dir '/sys/class/power_supply'");
    return;
  }
  struct dirent *ep;
  *sys_cls_pwr_bat = 0;
  while ((ep = readdir(dp))) {
    if (ep->d_name[0] == '.') {
      // skip hidden files
      continue;
    }
    // find out if type is battery
    char buf[512];
    if (snprintf(buf, sizeof(buf), "/sys/class/power_supply/%s/type",
                 ep->d_name) == sizeof(buf)) {
      printf("%s:%d - buffer probably overrun\n", __FILE__, __LINE__);
    }
    get_sys_value_str_tolower(buf, buf, sizeof(buf));
    if (strcmp(buf, "battery")) {
      continue;
    }
    // found 'battery'
    strncpy(sys_cls_pwr_bat, ep->d_name, sizeof(sys_cls_pwr_bat));

    //? quirk if it energy_full_design  charge_full_design
    if (snprintf(buf, sizeof(buf), "/sys/class/power_supply/%s/energy_now",
                 sys_cls_pwr_bat) == sizeof(buf)) {
      printf("%s %d - buffer probably overrun\n", __FILE__, __LINE__);
    }
    if (sys_value_exists(buf)) {
      battery_energy_or_charge_prefix = "energy";
      break;
    }
    if (snprintf(buf, sizeof(buf), "/sys/class/power_supply/%s/charge_now",
                 sys_cls_pwr_bat) == sizeof buf) {
      printf("%s %d - buffer probably overrun\n", __FILE__, __LINE__);
    }
    if (sys_value_exists(buf)) {
      battery_energy_or_charge_prefix = "charge";
      break;
    }
    printf("%s %d - energy or charge not resolved\n", __FILE__, __LINE__);
    break;
  }
  closedir(dp);
  if (!*sys_cls_pwr_bat) {
    puts("[!] no battery found in /sys/class/power_supply");
  }
  printf("· graphs battery: ");
  puts(sys_cls_pwr_bat);
  return;
}

static int is_wlan_device(const char *sys_cls_net_wlan) {
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
  struct stat s;
  if (stat(sb.chars, &s)) {
    return 0;
  }
  return 1;
}

static void auto_config_wifi() {
  DIR *dp = opendir("/sys/class/net");
  if (!dp) {
    puts("[!] wifi: cannot open find dir /sys/class/net");
    return;
  }
  struct dirent *ep;
  *sys_cls_net_wlan = 0;
  while ((ep = readdir(dp))) {
    if (ep->d_name[0] == '.') {
      // ignore hidden files
      continue;
    }
    if (!is_wlan_device(ep->d_name)) {
      continue;
    }
    // found wlan device
    strncpy(sys_cls_net_wlan, ep->d_name, sizeof(sys_cls_net_wlan));
    break;
  }
  closedir(dp);
  if (!*sys_cls_net_wlan) {
    puts("[!] no wireless device found in /sys/class/net");
  }
  printf("· graphs network device: ");
  puts(sys_cls_net_wlan);
  return;
}

static void auto_config() {
  auto_config_bat();
  auto_config_wifi();
}

static void on_draw() {
  dc_set_y(dc, Y_TOP);
  dc_clear(dc);
  _rend_datetime();
  _rend_cpu_load();
  _rend_hello_clonky();
  _rend_mem_info();
  _rend_swaps();
  _rend_wifi_traffic();
  net_main(dc);
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
  if (graphcpu) {
    graph_del(graphcpu);
  }
  if (graphmem) {
    graph_del(graphmem);
  }
  if (graphwifi) {
    graphd_del(graphwifi);
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
  graphcpu = graph_new(WIDTH);
  graphmem = graph_new(WIDTH);
  graphwifi = graphd_new(WIDTH);

  auto_config();

  while (1) {
    sleep(1);
    on_draw();
  }
}

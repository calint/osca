#include "dc.h"
#include "graph.h"
#include "graphd.h"
#include "main-cfg.h"
#include "strb.h"
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

// device context (renderer)
static struct dc *dc;

// cpu usage graph
static struct graph *graph_cpu;

// memory usage graph
static struct graph *graph_mem;

// network traffic graph
static struct graphd *graph_net;

// prefix to battery status directory
static const char *power_supply_path_prefix = "/sys/class/power_supply/";

// quirk for different names for battery charge indicator
static const char *battery_energy_or_charge_prefix = "";

// auto_config_battery() copies the battery entry in '/sys/class/power_supply/'
static char battery_name[32] = "";

// auto_config_network_traffic() copies the device name found in
// '/sys/class/net/' preferring wlan otherwise fallback to non loop-back device
static char net_device[32] = "";
static char net_device_is_wlan = 0;

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

static void str_to_lower(char *str) {
  while (*str) {
    *str = (char)tolower(*str);
    str++;
  }
}

// returns string value from /sys/ file system without new line or "" if
// anything goes wrong
// path: full path of file
// value: destination is written to pointer
// value_buf_size: size of value buffer
// if value is without '\n' at the end then write "" to value
static void sys_value_str_line(const char *path, char *value,
                               const unsigned value_buf_size) {
  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    value[0] = '\0';
    return;
  }
  ssize_t nbytes = read(fd, value, value_buf_size);
  if (nbytes <= 0) {
    value[0] = '\0';
    close(fd);
    return;
  }
  close(fd);
  if (value[nbytes - 1] == '\n') {
    value[nbytes - 1] = '\0';
    return;
  }
  value[0] = '\0';
}

static int sys_value_int(const char *path) {
  char str[64] = "";
  sys_value_str_line(path, str, sizeof(str));

  char *endptr = NULL;
  long result = strtol(str, &endptr, 10);
  if (endptr == str) {
    return 0;
  }
  return (int)result;
}

static long long sys_value_long(const char *path) {
  char str[64] = "";
  sys_value_str_line(path, str, sizeof(str));

  char *endptr = NULL;
  long long result = strtoll(str, &endptr, 10);
  if (endptr == str) {
    return 0;
  }
  return result;
}

static int sys_value_exists(const char *path) { return !access(path, F_OK); }

static void str_compact_spaces(char *str) {
  // "   a  b c  "
  char *dst = str;
  while (*str == ' ' || *str == '\t') {
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
    while (*str == ' ' || *str == '\t') {
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
  if (battery_name[0] == '\0') {
    // if no battery on the system
    return;
  }
  char path[128] = "";
  const int nchars =
      snprintf(path, sizeof(path), "%s%s/%s_", power_supply_path_prefix,
               battery_name, battery_energy_or_charge_prefix);
  if (nchars < 0 || (size_t)nchars >= sizeof(path)) {
    return; // truncated
  }
  const size_t maxlen = sizeof(path) - (size_t)nchars;
  // construct paths to '..._full' and '..._now'
  char *path_prefix_end = path + nchars;
  strncpy(path_prefix_end, "full", maxlen);
  path[sizeof(path) - 1] = '\0'; // ensure null-termination
  const long long charge_full = sys_value_long(path);
  strncpy(path_prefix_end, "now", maxlen);
  path[sizeof(path) - 1] = '\0'; // ensure null-termination
  const long long charge_now = sys_value_long(path);
  snprintf(path, sizeof(path), "%s%s/status", power_supply_path_prefix,
           battery_name);
  // read battery status
  char status[64];
  sys_value_str_line(path, status, sizeof(status));
  str_to_lower(status);
  // format output
  char output[128];
  snprintf(output, sizeof(output), "battery %u%%  %s",
           (unsigned)(charge_now * 100 / charge_full), status);
  // draw a separator for visual que of current battery charge
  dc_draw_hr1(dc, (int)(WIDTH * charge_now / charge_full));
  dc_newline(dc);
  dc_draw_str(dc, output);
}

static void render_cpu_load(void) {
  // previous reading
  static long long cpu_total_prv = 0;
  static long long cpu_usage_prv = 0;

  FILE *file = fopen("/proc/stat", "r");
  if (!file)
    return;
  // /proc/stat first line gives:
  //  cpu  20254782 11358 9292446 480440419 187402 0 2833065 0 0 0
  //   user: normal processes executing in user mode
  //   nice: niced processes executing in user mode
  //   system: processes executing in kernel mode
  //   idle: twiddling thumbs
  //   iowait: waiting for I/O to complete
  //   irq: servicing interrupts
  //   softirq: servicing softirqs
  long long user = 0;
  long long nice = 0;
  long long system = 0;
  long long idle = 0;
  long long iowait = 0;
  long long irq = 0;
  long long softirq = 0;
  char buf[32] = "";
  // read first line
  fscanf(file, "%31s %lld %lld %lld %lld %lld %lld %lld\n", buf, &user, &nice,
         &system, &idle, &iowait, &irq, &softirq);
  fclose(file);
  const long long total = user + nice + system + idle + iowait + irq + softirq;
  const long long usage = total - idle;
  const long long dtotal = total - cpu_total_prv;
  cpu_total_prv = total;
  const long long dusage = usage - cpu_usage_prv;
  cpu_usage_prv = usage;
  const long long usage_percent = dusage * 100 / dtotal;
  graph_add_value(graph_cpu, usage_percent);
  dc_inc_y(dc, HR_PIXELS_BEFORE + DEFAULT_GRAPH_HEIGHT);
  graph_draw(graph_cpu, dc, DEFAULT_GRAPH_HEIGHT, 100);
}

static void render_hello_clonky(void) {
  static long long unsigned counter = 0;
  counter++;
  char buf[128];
  snprintf(buf, sizeof(buf), "%llu hello%sclonky", counter,
           counter != 1 ? "s " : " ");
  dc_newline(dc);
  dc_draw_str(dc, buf);
}

static void render_mem_info(void) {
  FILE *file = fopen("/proc/meminfo", "r");
  if (!file) {
    return;
  }
  // /proc/meminfo gives:
  //  MemTotal:       15766756 kB
  //  MemFree:         4058344 kB
  //  MemAvailable:   10814308 kB
  //  Buffers:          944396 kB
  //  Cached:          5425168 kB
  char name[32] = "";
  char unit[16] = "";
  long long mem_total = 0;
  long long mem_avail = 0;
  char buf[256] = "";
  fgets(buf, sizeof(buf), file); //	MemTotal:        1937372 kB
  sscanf(buf, "%31s %lld %15s", name, &mem_total, unit);
  fgets(buf, sizeof(buf), file); //	MemFree:           99120 kB
  fgets(buf, sizeof(buf), file); //	MemAvailable:     887512 kB
  fclose(file);

  sscanf(buf, "%31s %lld %15s", name, &mem_avail, unit);
  const int proc = (int)((mem_total - mem_avail) * 100 / mem_total);
  graph_add_value(graph_mem, proc);
  dc_inc_y(dc, HR_PIXELS_BEFORE + DEFAULT_GRAPH_HEIGHT);
  graph_draw(graph_mem, dc, DEFAULT_GRAPH_HEIGHT, 100);
  if (mem_avail >> 10 != 0) {
    mem_avail >>= 10;
    mem_total >>= 10;
    strcpy(unit, "MB"); //? kB to MB not same as KB to MBs
  }
  snprintf(buf, sizeof(buf), "freemem %llu of %llu %s", mem_avail, mem_total,
           unit);
  dc_newline(dc);
  dc_draw_str(dc, buf);
}

static void render_net_graph(void) {
  if (net_device[0] == '\0') {
    // if no network device to graph
    return;
  }
  dc_inc_y(dc, DEFAULT_GRAPH_HEIGHT + HR_PIXELS_BEFORE);
  char path[128] = "";
  snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/tx_bytes",
           net_device);
  long long tx_bytes = sys_value_long(path);
  snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/rx_bytes",
           net_device);
  long long rx_bytes = sys_value_long(path);
  graphd_add_value(graph_net, tx_bytes + rx_bytes);
  graphd_draw(graph_net, dc, DEFAULT_GRAPH_HEIGHT, NET_GRAPH_MAX);
}

static void pl(const char *str) {
  dc_newline(dc);
  dc_draw_str(dc, str);
}

static void render_cheetsheet(void) {
  static char *keysheet[] = {"ĸey",
                             "+c               console",
                             "+f                 files",
                             "+e                editor",
                             "+m                 media",
                             "+v                 mixer",
                             "+i              internet",
                             "+x                sticky",
                             "+o              binaries",
                             "+p              snapshot",
                             "+shift+p          ↳ area",
                             "",
                             "đesktop",
                             "+up                   up",
                             "+down               down",
                             "",
                             "window",
                             "+esc               close",
                             "+b                  bump",
                             "+s                center",
                             "+w                 wider",
                             "+W               thinner",
                             "+r                resize",
                             "+1           full-screen",
                             "+2           full-height",
                             "+3            full-width",
                             "+right        focus-next",
                             "+left         focus-prev",
                             "+shift+up        move-up",
                             "+shift+down    move-down",
                             "+0   i-am-bored-surprise",
                             " ...                ... ",
                             NULL};

  char **str_ptr = keysheet;
  while (*str_ptr) {
    dc_newline(dc);
    dc_draw_str(dc, *str_ptr);
    str_ptr++;
  }
}

static void render_df(void) {
  FILE *file = popen("df -h 2> /dev/null", "r");
  if (!file) {
    return;
  }
  // df -h gives:
  //  Filesystem      Size  Used Avail Use% Mounted on
  //  tmpfs           1,6G  2,3M  1,6G   1% /run
  //  /dev/nvme0n1p6  235G  166G   57G  75% /

  char buf[256] = "";
  unsigned lines = 64; // arbitrary limit
  while (lines--) {
    if (fscanf(file, "%255[^\n]%*c", buf) == EOF) {
      break;
    }
    str_compact_spaces(buf);
    if (buf[0] != '/') {
      continue;
    }
    pl(buf);
  }
  pclose(file);
}

static void render_io_stat(void) {
  static long long prv_kb_read_total = 0;
  static long long prv_kb_written_total = 0;

  FILE *file = popen("iostat -d", "r");
  if (!file) {
    return;
  }
  // Linux 6.2.0-34-generic (c) 	2023-10-20 	_x86_64_	(12 CPU)
  // Device  tps  kB_read/s  kB_wrtn/s  kB_dscd/s  kB_read  kB_wrtn  kB_dscd
  //
  // loop0   0,00 0,00       0,00       0,00       21       0        0

  char buf[256] = "";
  // read the header
  fgets(buf, sizeof(buf), file);
  fgets(buf, sizeof(buf), file);
  fgets(buf, sizeof(buf), file);
  // sum values
  long long kb_read = 0;
  long long kb_read_total = 0;
  long long kb_written = 0;
  long long kb_written_total = 0;
  while (fgets(buf, sizeof(buf), file)) {
    if (buf[0] == '\0') {
      // break on empty line
      break;
    }
    char dev[64] = "";
    sscanf(buf, "%63s %*s %*s %*s %*s %lld %lld", dev, &kb_read, &kb_written);
    kb_read_total += kb_read;
    kb_written_total += kb_written;
  }
  pclose(file);

  const char *unit = "kB/s";
  snprintf(buf, sizeof(buf), "read %lld %s wrote %lld %s",
           kb_read_total -
               (prv_kb_read_total ? prv_kb_read_total : kb_read_total),
           unit,
           kb_written_total -
               (prv_kb_written_total ? prv_kb_written_total : kb_written_total),
           unit);
  pl(buf);

  prv_kb_read_total = kb_read_total;
  prv_kb_written_total = kb_written_total;
}

static void render_syslog(void) {
  FILE *file = popen("journalctl -b -p6 -o cat -n 15 --no-pager", "r");
  if (!file) {
    return;
  }
  char buf[512];
  unsigned counter = 15; // maximum 15 line
  while (counter--) {
    if (fscanf(file, "%511[^\n]%*c", buf) == EOF) {
      break;
    }
    pl(buf);
  }
  pclose(file);
}

static void render_acpi(void) {
  FILE *file = popen("acpi -at", "r");
  if (!file) {
    return;
  }
  // no-infinite loop, arbitrary limit
  unsigned counter = 64;
  while (counter--) {
    char buf[512] = "";
    // read at most 511 characters before a newline and read the newline
    // characters
    if (fscanf(file, "%511[^\n]%*c", buf) == EOF) {
      break;
    }
    str_to_lower(buf);
    pl(buf);
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

static void render_cores_throttle(void) {
  FILE *file = fopen("/sys/devices/system/cpu/present", "r");
  if (!file) {
    return;
  }
  unsigned min = 0;
  unsigned max = 0;
  fscanf(file, "%u-%u", &min, &max);
  fclose(file);

  strb sb;
  strb_init(&sb);
  if (strb_p(&sb, "throttle ")) {
    return;
  }
  const unsigned ncpus = max - min + 1;
  strb_p_int(&sb, (int)ncpus);
  strb_p(&sb, " core");
  if (ncpus != 1) {
    strb_p_char(&sb, 's');
  }
  if (ncpus > 2) {
    // if more than 2 cpus display throttles on new line
    pl(sb.chars);
    // puts(sb.chars);
    strb_clear(&sb);
  }
  const unsigned ncols = 5;
  unsigned cpu_ix = min;
  while (cpu_ix <= max) {
    for (unsigned col = 0; col < ncols && cpu_ix <= max; col++) {
      char path[128] = "";
      snprintf(path, sizeof(path),
               "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq",
               cpu_ix);
      const long long max_freq = sys_value_long(path);
      snprintf(path, sizeof(path),
               "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq",
               cpu_ix);
      const long long cur_freq = sys_value_long(path);
      strb_p_char(&sb, ' ');
      if (max_freq) {
        // if available render percent of max frequency
        const unsigned proc = (unsigned)(cur_freq * 100 / max_freq);
        strb_p_int_with_width(&sb, (int)proc, 3);
        strb_p_char(&sb, '%');
      } else {
        // max frequency not available
        strb_p(&sb, "----");
      }
      cpu_ix++;
    }
    pl(sb.chars);
    // puts(sb.chars);
    strb_clear(&sb);
  }
}

static void render_swaps(void) {
  FILE *file = fopen("/proc/swaps", "r");
  if (!file) {
    return;
  }
  // Filename         Type       Size     Used   Priority
  // /dev/mmcblk0p3   partition  2096124  16568  s-1
  char buf[256] = "";
  // read column headers
  fgets(buf, sizeof(buf), file);
  long long size_kb = 0;
  long long used_kb = 0;
  if (!fscanf(file, "%*s %*s %lld %lld", &size_kb, &used_kb)) {
    fclose(file);
    return;
  }
  fclose(file);

  strb sb;
  strb_init(&sb);
  if (strb_p(&sb, "swapped ")) {
    return;
  }
  if (strb_p_nbytes(&sb, used_kb << 10)) {
    return;
  }
  pl(sb.chars);
}

static void auto_config_battery(void) {
  DIR *dir = opendir("/sys/class/power_supply");
  if (!dir) {
    puts("[!] battery: cannot open dir '/sys/class/power_supply'");
    return;
  }
  struct dirent *entry = NULL;
  battery_name[0] = '\0';
  while ((entry = readdir(dir))) {
    if (entry->d_name[0] == '.') {
      // skip hidden files
      continue;
    }
    // find out if type is battery
    char buf[128] = "";
    snprintf(buf, sizeof(buf), "/sys/class/power_supply/%.*s/type", 64,
             entry->d_name);
    sys_value_str_line(buf, buf, sizeof(buf));
    str_to_lower(buf);
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
  if (battery_name[0] == '\0') {
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
  struct stat st; //? initial value
  return !stat(sb.chars, &st);
}

static void auto_config_network_traffic(void) {
  DIR *dir = opendir("/sys/class/net");
  if (!dir) {
    puts("[!] wifi: cannot open dir /sys/class/net");
    return;
  }
  struct dirent *entry = NULL;
  net_device[0] = '\0';
  while ((entry = readdir(dir))) {
    if (entry->d_name[0] == '.') {
      // ignore hidden files
      continue;
    }
    if (!strcmp("lo", entry->d_name)) {
      // skip loopback
      continue;
    }
    if (is_wlan_device(entry->d_name)) {
      // found wlan device (preferred)
      strncpy(net_device, entry->d_name, sizeof(net_device));
      net_device_is_wlan = 1;
      break;
    }
    // network device (not preferred)
    strncpy(net_device, entry->d_name, sizeof(net_device));
  }
  closedir(dir);
  if (!net_device[0]) { //? map 'lo'?
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
  for (unsigned i = 0; i < netifcs_len; i++) {
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

static void render_wifi_info_for_interface(const char *interface_name) {
  // :: iw dev wlan0 link
  // Connected to 38:d5:47:40:99:d4 (on wlan0)
  //         SSID: AC51_5G
  //         freq: 5180.0
  //         RX: 712799364 bytes (565282 packets)
  //         TX: 26835925 bytes (182863 packets)
  //         signal: -75 dBm
  //         rx bitrate: 234.0 MBit/s VHT-MCS 5 80MHz VHT-NSS 1
  //         tx bitrate: 150.0 MBit/s VHT-MCS 7 40MHz short GI VHT-NSS 1
  //         bss flags: short-slot-time
  //         dtim period: 1
  //         beacon int: 100
  // ::
  char cmd[256] = "";
  snprintf(cmd, sizeof(cmd), "iw dev %s link", interface_name);
  FILE *file = popen(cmd, "r");
  if (!file) {
    return;
  }
  // discard first line
  fgets(cmd, sizeof(cmd), file);
  // read SSID
  char ssid[128] = "";
  if (!fscanf(file, "\tSSID: %127[^\n]%*c", ssid)) {
    return;
  }
  if (ssid[0] == '\0') {
    return;
  }
  // discard next three lines
  fgets(cmd, sizeof(cmd), file);
  fgets(cmd, sizeof(cmd), file);
  fgets(cmd, sizeof(cmd), file);
  // read signal strength
  char signal[64] = "";
  if (!fscanf(file, "\tsignal: %63[^\n]%*c", signal)) {
    return;
  }
  pclose(file);
  // re-use 'cmd' buffer to print the result
  snprintf(cmd, sizeof(cmd), " %s   %s", ssid, signal);
  pl(cmd);
}

static void render_net_interface(struct ifaddrs *ifa) {
  if (ifa->ifa_addr == NULL || (ifa->ifa_addr->sa_family != AF_INET &&
                                ifa->ifa_addr->sa_family != AF_INET6)) {
    return;
  }

  char ip_addr[NI_MAXHOST] = "";
  if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), ip_addr,
                  NI_MAXHOST, NULL, 0, NI_NUMERICHOST)) {
    return;
  }

  // get operational status of interface
  char path[128] = "";
  snprintf(path, sizeof(path), "/sys/class/net/%.*s/operstate", 32,
           ifa->ifa_name);
  char operstate[32] = "";
  sys_value_str_line(path, operstate, sizeof(operstate));
  str_to_lower(operstate);
  if (!strcmp(operstate, "unknown")) {
    operstate[0] = '\0';
  }

  char buf[256] = "";
  snprintf(buf, sizeof(buf), "%.*s: %.*s %.*s", 32, ifa->ifa_name, 64, ip_addr,
           (int)sizeof(operstate), operstate);
  pl(buf);

  // is it the wifi device?
  if (net_device_is_wlan &&
      !strncmp(ifa->ifa_name, net_device, sizeof(net_device))) {
    // yes. print network name and signal strength using command:
    render_wifi_info_for_interface(ifa->ifa_name);
  }

  // get stats from /sys
  snprintf(path, sizeof(path), "/sys/class/net/%.*s/statistics/tx_bytes", 32,
           ifa->ifa_name);
  long long tx_bytes = sys_value_long(path);
  snprintf(path, sizeof(path), "/sys/class/net/%.*s/statistics/rx_bytes", 32,
           ifa->ifa_name);
  long long rx_bytes = sys_value_long(path);
  // get or create entry
  struct netifc *ifc = netifcs_get_by_name_or_create(ifa->ifa_name);
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

  snprintf(buf, sizeof(buf), " ↓ %lld %s ↑ %lld %s", delta_rx_bytes, rx_scale,
           delta_tx_bytes, tx_scale);
  pl(buf);
}

static void render_net_interfaces(void) {
  struct ifaddrs *ifas = NULL;
  if (getifaddrs(&ifas) == -1) {
    return;
  }

  // first the graphed device
  for (struct ifaddrs *ifa = ifas; ifa != NULL; ifa = ifa->ifa_next) {
    if (strncmp(ifa->ifa_name, net_device, sizeof(net_device))) {
      continue;
    }
    render_net_interface(ifa);
    // not 'break' because the 'ifaddrs' have multiple entries while and only
    // one of those entries gives result from 'getnameinfo'
  }
  // then all others except 'lo'
  for (struct ifaddrs *ifa = ifas; ifa != NULL; ifa = ifa->ifa_next) {
    if (!strncmp(ifa->ifa_name, net_device, sizeof(net_device)) ||
        !strncmp(ifa->ifa_name, "lo", sizeof("lo"))) {
      continue;
    }
    render_net_interface(ifa);
    // not 'break' because see note above
  }
  // then 'lo'
  for (struct ifaddrs *ifa = ifas; ifa != NULL; ifa = ifa->ifa_next) {
    if (strncmp(ifa->ifa_name, "lo", sizeof("lo"))) {
      continue;
    }
    render_net_interface(ifa);
    // not 'break' because see note above
  }
  freeifaddrs(ifas);
}

static void render_top_10_processes(void) {
  FILE *file = popen("ps -eo %mem,%cpu,comm --sort=-%mem | head -n 11", "r");
  if (!file) {
    return;
  }
  char buf[512];
  unsigned counter = 11;
  while (counter--) {
    if (fscanf(file, "%511[^\n]%*c", buf) == EOF) {
      break;
    }
    pl(buf);
  }
  pclose(file);
}

static void signal_exit(int i) {
  puts("\nexiting");
  dc_del(/*gives*/ dc);
  if (graph_cpu) {
    graph_del(/*gives*/ graph_cpu);
  }
  if (graph_mem) {
    graph_del(/*gives*/ graph_mem);
  }
  if (graph_net) {
    graphd_del(/*gives*/ graph_net);
  }
  exit(i);
}

static void render(void) {
  dc_clear(dc);
  render_date_time();
  render_cpu_load();
  render_hello_clonky();
  render_mem_info();
  render_swaps();
  render_net_graph();
  render_net_interfaces();
  render_hr();
  render_io_stat();
  render_df();
  render_hr();
  render_cores_throttle();
  render_battery();
  render_acpi();
  //  render_hr();
  //  render_top_10_processes();
  render_hr();
  render_syslog();
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

  dc = /*takes*/ dc_new(FONT_NAME, FONT_SIZE, LINE_HEIGHT, MARGIN_TOP, WIDTH,
                        HR_PIXELS_BEFORE, HR_PIXELS_AFTER, ALIGN);
  graph_cpu = /*takes*/ graph_new(WIDTH);
  graph_mem = /*takes*/ graph_new(WIDTH);
  graph_net = /*takes*/ graphd_new(WIDTH);

  auto_config();

  while (1) {
    sleep(1);
    render();
  }
}

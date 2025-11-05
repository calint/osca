//
// reviewed: 2025-02-22
// reviewed: 2025-02-24
// reviewed: 2025-02-25
//
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

// device context (renderer)
static struct dc* dc;

// cpu usage graph
static struct graph* graph_cpu;

// memory usage graph
static struct graph* graph_mem;

// network traffic graph
static struct graphd* graph_net;

// quirk for different names for battery charge indicator
// set at 'auto_config_battery'
static const char* battery_energy_or_charge_prefix;

// 'auto_config_battery' copies the battery entry in '/sys/class/power_supply/'
static char battery_name[32];

// 'auto_config_network_traffic' copies the device name found in
// '/sys/class/net/' preferring wifi otherwise fallback to non loop-back device
static char net_device[32];
static char net_device_is_wifi;

// network interfaces
static struct netifc {
    char name[NETIFC_NAME_SIZE];
    // name as listed in '/sys/class/net/'

    uint64_t rx_bytes;
    // total bytes received

    uint64_t tx_bytes;
    // total bytes transmitted
} netifcs[NETIFC_ARRAY_SIZE];

// used network interfaces
static uint32_t netifcs_len;

static void str_to_lower(char* str) {
    while (*str) {
        *str = (char)tolower(*str);
        str++;
    }
}

static void str_compact_spaces(char* str) {
    char* dst = str;
    // "   a  b c  "
    while (isspace(*str)) {
        str++;
    }
    // "a  b c  "
    while (1) {
        *dst++ = *str;
        if (*str == '\0') {
            return;
        }
        str++;
        uint32_t is_spc = 0;
        while (isspace(*str)) {
            str++;
            is_spc++;
        }
        if (is_spc && *str != '\0') {
            *dst++ = ' ';
        }
    }
}

// returns string value from '/sys/' file system without new line or "" if
// anything goes wrong
// path: full path of file
// value: destination is written to pointer
// value_buf_size: size of value buffer
// if value is without '\n' at the end then write "" to value
static void sys_value_str_line(const char* path, char* value,
                               const size_t value_buf_size) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        value[0] = '\0';
        return;
    }
    const ssize_t nbytes = read(fd, value, value_buf_size);
    close(fd);
    if (nbytes <= 0) {
        value[0] = '\0';
        return;
    }
    if (value[nbytes - 1] != '\n') {
        value[0] = '\0';
        return;
    }
    value[nbytes - 1] = '\0';
}

static int32_t sys_value_int32(const char* path) {
    char str[64] = "";
    sys_value_str_line(path, str, sizeof(str));

    char* endptr = NULL;
    const long result = strtol(str, &endptr, 10);
    if (endptr == str) {
        return 0;
    }
    return (int32_t)result;
}

static int64_t sys_value_int64(const char* path) {
    char str[64] = "";
    sys_value_str_line(path, str, sizeof(str));

    char* endptr = NULL;
    const long long result = strtoll(str, &endptr, 10);
    if (endptr == str) {
        return 0;
    }
    return (int64_t)result;
}

static uint64_t sys_value_uint64(const char* path) {
    char str[64] = "";
    sys_value_str_line(path, str, sizeof(str));

    char* endptr = NULL;
    const unsigned long long result = strtoull(str, &endptr, 10);
    if (endptr == str) {
        return 0;
    }
    return (uint64_t)result;
}

static int sys_value_exists(const char* path) { return !access(path, F_OK); }

static void auto_config_battery(void) {
    DIR* dir = opendir("/sys/class/power_supply");
    if (!dir) {
        puts("[!] battery: cannot open dir '/sys/class/power_supply'");
        return;
    }
    struct dirent* entry = NULL;
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
        // note: 64 is arbitrary max length of entry name
        sys_value_str_line(buf, buf, sizeof(buf));
        str_to_lower(buf);
        if (strcmp(buf, "battery")) {
            // note: length of "battery" is less than 'buf'
            continue;
        }
        // found 'battery'
        strncpy(battery_name, entry->d_name, sizeof(battery_name));
        battery_name[sizeof(battery_name) - 1] = '\0';
        // note: ensure null-termination

        //? quirk if it is 'energy' or 'charge' for battery charge info
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
        puts("[!] no battery found in /sys/class/power_supply/");
    }
    printf("· battery: ");
    puts(battery_name);
    return;
}

static int is_wifi_device(const char* sys_cls_net_wlan) {
    // build string to file '/sys/class/net/XXX/wireless
    char buf[128] = "";
    snprintf(buf, sizeof(buf), "/sys/class/net/%s/wireless", sys_cls_net_wlan);
    struct stat st = {0};
    return !stat(buf, &st);
}

static void auto_config_network_traffic(void) {
    DIR* dir = opendir("/sys/class/net");
    if (!dir) {
        puts("[!] wifi: cannot open dir /sys/class/net");
        return;
    }
    net_device[0] = '\0';
    struct dirent* entry = NULL;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') {
            // ignore hidden files
            continue;
        }
        if (!strcmp(entry->d_name, "lo")) {
            // note: length of "lo" is less than sizeof 'entry->d_name'
            // skip loopback
            continue;
        }
        if (is_wifi_device(entry->d_name)) {
            // found wifi device (preferred)
            strncpy(net_device, entry->d_name, sizeof(net_device));
            net_device[sizeof(net_device) - 1] =
                '\0'; // ensure null-termination
            net_device_is_wifi = 1;
            break;
        }
        // network device (not preferred)
        strncpy(net_device, entry->d_name, sizeof(net_device));
        net_device[sizeof(net_device) - 1] = '\0'; // ensure null-termination
    }
    closedir(dir);
    if (net_device[0] == '\0') {
        puts("[!] no network device found in /sys/class/net/");
    }
    printf("· graph network interface: ");
    puts(net_device);
    return;
}

static void auto_config(void) {
    auto_config_battery();
    auto_config_network_traffic();
}

static void pl(const char* str) {
    dc_newline(dc);
    dc_draw_str(dc, str);
}

static void render_hr(void) { dc_draw_hr(dc); }

static void render_date_time(void) {
    const time_t t = time(NULL);
    const struct tm* lt = localtime(&t);
    strb sb;
    strb_init(&sb);
    if (strb_p(&sb, asctime(lt))) {
        return;
    }
    // delete the '\n' that 'asctime' writes at the end of the string
    strb_back(&sb);
    pl(sb.chars);
}

static void render_cpu_load(void) {
    // previous reading
    static uint64_t prv_total = 0;
    static uint64_t prv_usage = 0;

    FILE* file = fopen("/proc/stat", "r");
    if (!file) {
        return;
    }
    // /proc/stat first line gives:
    //  cpu  20254782 11358 9292446 480440419 187402 0 2833065 0 0 0
    //   user: normal processes executing in user mode
    //   nice: niced processes executing in user mode
    //   system: processes executing in kernel mode
    //   idle: twiddling thumbs
    //   iowait: waiting for I/O to complete
    //   irq: servicing interrupts
    //   softirq: servicing softirqs
    uint64_t user = 0;
    uint64_t nice = 0;
    uint64_t system = 0;
    uint64_t idle = 0;
    uint64_t iowait = 0;
    uint64_t irq = 0;
    uint64_t softirq = 0;
    // read first line
    if (fscanf(file, "cpu %lu %lu %lu %lu %lu %lu %lu\n", &user, &nice, &system,
               &idle, &iowait, &irq, &softirq) != 7) {
        fclose(file);
        return;
    }
    fclose(file);
    const uint64_t total = user + nice + system + idle + iowait + irq + softirq;
    const uint64_t dtotal = total - prv_total;
    prv_total = total;
    const uint64_t usage = total - idle;
    const uint64_t dusage = usage - prv_usage;
    prv_usage = usage;
    const uint64_t usage_percent = dtotal ? (dusage * 100 / dtotal) : 0;
    graph_add_value(graph_cpu, usage_percent);
    dc_inc_y(dc, HR_PIXELS_BEFORE + DEFAULT_GRAPH_HEIGHT);
    graph_draw(graph_cpu, dc, DEFAULT_GRAPH_HEIGHT, 100);
    // note: 100 for percent
}

static void render_hello_clonky(void) {
    static uint64_t counter = 0;
    char buf[128];
    snprintf(buf, sizeof(buf), "%lu hello%s clonky", counter,
             counter != 1 ? "s" : "");
    pl(buf);
    counter++;
}

static void render_mem_info(void) {
    FILE* file = fopen("/proc/meminfo", "r");
    if (!file) {
        return;
    }
    //  MemTotal:       15766756 kB
    //  MemFree:         4058344 kB
    //  MemAvailable:   10814308 kB
    //  Buffers:          944396 kB
    //  Cached:          5425168 kB
    const char* unit = "kB";
    uint64_t mem_total = 0;
    uint64_t mem_avail = 0;
    char buf[256] = "";
    fgets(buf, sizeof(buf), file); //	MemTotal:        1937372 kB
    if (sscanf(buf, "%*s %lu", &mem_total) != 1) {
        fclose(file);
        return;
    }
    fgets(buf, sizeof(buf), file); //	MemFree:           99120 kB
    fgets(buf, sizeof(buf), file); //	MemAvailable:     887512 kB
    fclose(file);
    if (sscanf(buf, "%*s %lu", &mem_avail) != 1) {
        return;
    }
    const uint64_t proc =
        mem_total ? ((mem_total - mem_avail) * 100 / mem_total) : 100;
    graph_add_value(graph_mem, proc);
    dc_inc_y(dc, HR_PIXELS_BEFORE + DEFAULT_GRAPH_HEIGHT);
    graph_draw(graph_mem, dc, DEFAULT_GRAPH_HEIGHT, 100);
    // note: 100 for percent
    if (mem_avail >> 10 != 0) {
        // note: no rounding to nearest integer to give minimum
        mem_avail >>= 10;
        mem_total >>= 10;
        unit = "MB";
    }
    snprintf(buf, sizeof(buf), "freemem %lu of %lu %s", mem_avail, mem_total,
             unit);
    pl(buf);
}

static void render_swaps(void) {
    FILE* file = fopen("/proc/swaps", "r");
    if (!file) {
        return;
    }
    // Filename         Type       Size     Used   Priority
    // /dev/mmcblk0p3   partition  2096124  16568  s-1
    char buf[256] = "";
    // read column headers
    fgets(buf, sizeof(buf), file);
    uint64_t size_kb = 0;
    uint64_t used_kb = 0;
    if (fscanf(file, "%*s %*s %lu %lu", &size_kb, &used_kb) != 2) {
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

static void render_net_graph(void) {
    if (net_device[0] == '\0') {
        // if no network device to graph
        dc_draw_hr(dc);
        // draw a line which otherwise is implied by the graph
        return;
    }
    dc_inc_y(dc, DEFAULT_GRAPH_HEIGHT + HR_PIXELS_BEFORE);
    char path[128] = "";
    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/tx_bytes",
             net_device);
    const uint64_t tx_bytes = sys_value_uint64(path);
    snprintf(path, sizeof(path), "/sys/class/net/%s/statistics/rx_bytes",
             net_device);
    const uint64_t rx_bytes = sys_value_uint64(path);
    graphd_add_value(graph_net, tx_bytes + rx_bytes);
    graphd_draw(graph_net, dc, DEFAULT_GRAPH_HEIGHT, NET_GRAPH_MAX);
}

static struct netifc* netifcs_get_by_name_or_create(const char* name) {
    for (uint32_t i = 0; i < netifcs_len; i++) {
        struct netifc* ni = &netifcs[i];
        if (!strncmp(ni->name, name, sizeof(ni->name))) {
            return &netifcs[i];
        }
    }
    if (netifcs_len >= NETIFC_ARRAY_SIZE) {
        // if no more devices fit in the array
        return NULL;
    }
    printf("· network interface: %s\n", name);
    struct netifc* ni = &netifcs[netifcs_len];
    netifcs_len++;
    strncpy(ni->name, name, sizeof(ni->name));
    ni->name[sizeof(ni->name) - 1] = '\0'; // ensure null-termination
    return ni;
}

static void render_wifi_info_for_interface(const char* interface_name) {
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
    FILE* file = popen(cmd, "r");
    if (!file) {
        return;
    }
    // discard first line
    fgets(cmd, sizeof(cmd), file);
    // read SSID
    char ssid[128] = "";
    if (fscanf(file, " SSID: %127[^\n]%*c", ssid) != 1) {
        // note: the leading space in format string ignores any whitespace
        pclose(file);
        return;
    }
    if (ssid[0] == '\0') {
        pclose(file);
        return;
    }
    // discard next three lines
    fgets(cmd, sizeof(cmd), file);
    fgets(cmd, sizeof(cmd), file);
    fgets(cmd, sizeof(cmd), file);
    // read signal strength
    char signal[64] = "";
    if (fscanf(file, " signal: %63[^\n]%*c", signal) != 1) {
        // note: the leading space in format string ignores any whitespace
        pclose(file);
        return;
    }
    pclose(file);
    // re-use 'cmd' buffer to print the result
    snprintf(cmd, sizeof(cmd), " %s   %s", ssid, signal);
    pl(cmd);
}

static void render_net_interface(struct ifaddrs* ifa) {
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
    // note: 32 is arbitrary max length of interface name

    char operstate[32] = "";
    sys_value_str_line(path, operstate, sizeof(operstate));
    str_to_lower(operstate);
    if (!strncmp(operstate, "unknown", sizeof(operstate))) {
        operstate[0] = '\0';
    }

    char buf[256] = "";
    snprintf(buf, sizeof(buf), "%.*s: %.*s %.*s", 32, ifa->ifa_name, 64,
             ip_addr, (int)sizeof(operstate), operstate);
    // note: 32 is arbitrary max length of interface name and 64 for ip address
    pl(buf);

    // is it the wifi device?
    if (net_device_is_wifi &&
        !strncmp(ifa->ifa_name, net_device, sizeof(net_device))) {
        // yes. print network name and signal strength using command:
        render_wifi_info_for_interface(ifa->ifa_name);
    }

    // get stats from /sys
    snprintf(path, sizeof(path), "/sys/class/net/%.*s/statistics/tx_bytes", 32,
             ifa->ifa_name);
    // note: 32 is arbitrary max length of interface name
    const uint64_t tx_bytes = sys_value_uint64(path);
    snprintf(path, sizeof(path), "/sys/class/net/%.*s/statistics/rx_bytes", 32,
             ifa->ifa_name);
    // note: 32 is arbitrary max length of interface name
    const uint64_t rx_bytes = sys_value_uint64(path);
    // get or create entry
    struct netifc* ifc = netifcs_get_by_name_or_create(ifa->ifa_name);
    if (!ifc) {
        // no more space in the network interfaces array
        return;
    }
    uint64_t delta_tx_bytes =
        tx_bytes - (ifc->tx_bytes ? ifc->tx_bytes : tx_bytes);
    uint64_t delta_rx_bytes =
        rx_bytes - (ifc->rx_bytes ? ifc->rx_bytes : rx_bytes);

    ifc->tx_bytes = tx_bytes;
    ifc->rx_bytes = rx_bytes;

    // ? round to nearest integer
    const char* rx_scale = "B/s";
    if (delta_rx_bytes >> 20) {
        delta_rx_bytes >>= 20;
        rx_scale = "MB/s";
    } else if (delta_rx_bytes >> 10) {
        delta_rx_bytes >>= 10;
        rx_scale = "KB/s";
    }

    const char* tx_scale = "B/s";
    if (delta_tx_bytes >> 20) {
        delta_tx_bytes >>= 20;
        tx_scale = "MB/s";
    } else if (delta_tx_bytes >> 10) {
        delta_tx_bytes >>= 10;
        tx_scale = "KB/s";
    }

    snprintf(buf, sizeof(buf), " ↓ %lu %s ↑ %lu %s", delta_rx_bytes, rx_scale,
             delta_tx_bytes, tx_scale);
    pl(buf);
}

static void render_net_interfaces(void) {
    struct ifaddrs* ifas = NULL;
    if (getifaddrs(&ifas) == -1) {
        return;
    }

    // ? implement new algorithm to make one pass through the list of interfaces
    // first the graphed device
    for (struct ifaddrs* ifa = ifas; ifa != NULL; ifa = ifa->ifa_next) {
        if (strncmp(ifa->ifa_name, net_device, sizeof(net_device))) {
            continue;
        }
        render_net_interface(ifa);
        // not 'break' because the 'ifaddrs' have multiple entries while and
        // only one of those entries gives result from 'getnameinfo'
    }
    // then all others except 'lo'
    for (struct ifaddrs* ifa = ifas; ifa != NULL; ifa = ifa->ifa_next) {
        if (!strncmp(ifa->ifa_name, net_device, sizeof(net_device)) ||
            !strncmp(ifa->ifa_name, "lo", sizeof("lo"))) {
            continue;
        }
        render_net_interface(ifa);
        // not 'break' because see note above
    }
    // then 'lo'
    for (struct ifaddrs* ifa = ifas; ifa != NULL; ifa = ifa->ifa_next) {
        if (strncmp(ifa->ifa_name, "lo", sizeof("lo"))) {
            continue;
        }
        render_net_interface(ifa);
        // not 'break' because see note above
    }
    freeifaddrs(ifas);
}

static void render_io_stat(void) {
    static uint64_t prv_kb_read_total = 0;
    static uint64_t prv_kb_written_total = 0;

    FILE* file = popen("iostat -d", "r");
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
    uint64_t kb_read_total = 0;
    uint64_t kb_written_total = 0;
    while (fgets(buf, sizeof(buf), file)) {
        if (buf[0] == '\n') {
            // break on empty line
            break;
        }
        uint64_t kb_read = 0;
        uint64_t kb_written = 0;
        if (sscanf(buf, "%*s %*s %*s %*s %*s %lu %lu", &kb_read, &kb_written) ==
            2) {
            kb_read_total += kb_read;
            kb_written_total += kb_written;
        }
    }
    pclose(file);

    snprintf(
        buf, sizeof(buf), "read %lu kB/s wrote %lu kB/s",
        kb_read_total - (prv_kb_read_total ? prv_kb_read_total : kb_read_total),
        kb_written_total -
            (prv_kb_written_total ? prv_kb_written_total : kb_written_total));
    pl(buf);

    prv_kb_read_total = kb_read_total;
    prv_kb_written_total = kb_written_total;
}

static void render_df(void) {
    FILE* file = popen("df -h", "r");
    if (!file) {
        return;
    }
    //  Filesystem      Size  Used Avail Use% Mounted on
    //  tmpfs           1,6G  2,3M  1,6G   1% /run
    //  /dev/nvme0n1p6  235G  166G   57G  75% /

    char buf[256] = "";
    uint32_t counter = RENDER_DF_MAX_LINE_COUNT;
    while (counter--) {
        if (fscanf(file, "%255[^\n]%*c", buf) == EOF) {
            break;
        }
        if (buf[0] != '/') {
            continue;
        }
        str_compact_spaces(buf);
        pl(buf);
    }
    pclose(file);
}

static int render_threads_throttle_visual_compare(const void* a,
                                                  const void* b) {
    const uint32_t val_a = *(uint32_t*)a;
    const uint32_t val_b = *(uint32_t*)b;

    if (val_b > val_a) {
        return 1; // descending
    }
    if (val_b < val_a) {
        return -1;
    }
    return 0;
}

static void render_threads_throttle_visual(void) {
    FILE* file = fopen("/sys/devices/system/cpu/present", "r");
    if (!file) {
        return;
    }
    uint32_t min = 0;
    uint32_t max = 0;
    if (fscanf(file, "%u-%u", &min, &max) != 2) {
        fclose(file);
        return;
    }
    fclose(file);

    const uint32_t nthreads = max - min + 1;

    uint32_t total_proc = 0;
    uint32_t threads_width[nthreads];

    for (uint32_t i = min, j = 0; i <= max; i++, j++) {
        char path[128] = "";
        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu%u/cpufreq/cpuinfo_max_freq", i);
        const uint64_t max_freq = sys_value_uint64(path);

        snprintf(path, sizeof(path),
                 "/sys/devices/system/cpu/cpu%u/cpufreq/scaling_cur_freq", i);
        const uint64_t cur_freq = sys_value_uint64(path);

        const uint32_t proc =
            max_freq ? (uint32_t)(cur_freq * 100 / max_freq) : 100;
        total_proc += proc;
        threads_width[j] = (uint32_t)(WIDTH * cur_freq / max_freq);
    }

    qsort(threads_width, nthreads, sizeof(uint32_t),
          render_threads_throttle_visual_compare);

    strb sb;
    strb_init(&sb);
    strb_p(&sb, "throttle ");
    strb_p_uint32(&sb, nthreads);
    strb_p(&sb, " thread");
    if (nthreads != 1) {
        strb_p_char(&sb, 's');
    }
    strb_p_char(&sb, ' ');
    strb_p_uint32(&sb, (total_proc + (nthreads / 2)) / nthreads);
    // note: nthreads / 2 for rounding to nearest integer
    strb_p_char(&sb, '%');
    pl(sb.chars);

    for (uint32_t i = 0; i < nthreads; i++) {
        dc_draw_hr1(dc, threads_width[i]);
    }
}

static void render_threads_throttle(void) {
    FILE* file = fopen("/sys/devices/system/cpu/present", "r");
    if (!file) {
        return;
    }
    uint32_t min = 0;
    uint32_t max = 0;
    if (fscanf(file, "%u-%u", &min, &max) != 2) {
        fclose(file);
        return;
    }
    fclose(file);

    strb sb;
    strb_init(&sb);
    if (strb_p(&sb, "throttle ")) {
        return;
    }
    const uint32_t ncpus = max - min + 1;
    strb_p_uint32(&sb, ncpus);
    strb_p(&sb, " thread");
    if (ncpus != 1) {
        strb_p_char(&sb, 's');
    }
    if (ncpus > 2) {
        // if more than 2 cpus display throttles on new line
        pl(sb.chars);
        strb_init(&sb);
    }
    const uint32_t ncols = 5;
    uint32_t cpu_ix = min;
    uint32_t total_proc = 0;
    while (cpu_ix <= max) {
        for (uint32_t col = 0; col < ncols && cpu_ix <= max; col++) {
            char path[128] = "";
            snprintf(path, sizeof(path),
                     "/sys/devices/system/cpu/cpu%u/cpufreq/cpuinfo_max_freq",
                     cpu_ix);
            const uint64_t max_freq = sys_value_uint64(path);
            snprintf(path, sizeof(path),
                     "/sys/devices/system/cpu/cpu%u/cpufreq/scaling_cur_freq",
                     cpu_ix);
            const uint64_t cur_freq = sys_value_uint64(path);
            strb_p_char(&sb, ' ');
            if (max_freq) {
                // if available render percent of max frequency
                const uint32_t proc =
                    max_freq ? (uint32_t)(cur_freq * 100 / max_freq) : 100;
                total_proc += proc;
                strb_p_uint32_with_width(&sb, proc, 3);
                strb_p_char(&sb, '%');
            } else {
                // max frequency not available
                strb_p(&sb, "----");
            }
            cpu_ix++;
        }
        pl(sb.chars);
        strb_init(&sb);
    }
    if (ncpus > 1 && total_proc != 0) {
        strb_p(&sb, "average: ");
        strb_p_uint32(&sb, (total_proc + (ncpus / 2)) / ncpus);
        // note: ncpus / 2 for rounding to nearest integer
        strb_p_char(&sb, '%');
        pl(sb.chars);
    }
}

static void render_battery(void) {
    if (battery_name[0] == '\0') {
        // if no battery on the system
        return;
    }

    char buf[256] = "";

    snprintf(buf, sizeof(buf), "/sys/class/power_supply/%s/%s_full",
             battery_name, battery_energy_or_charge_prefix);
    const uint64_t charge_full = sys_value_uint64(buf);

    snprintf(buf, sizeof(buf), "/sys/class/power_supply/%s/%s_now",
             battery_name, battery_energy_or_charge_prefix);
    const uint64_t charge_now = sys_value_uint64(buf);

    // draw a separator for visual que of current battery charge
    dc_draw_hr1(dc, charge_full ? (uint32_t)(WIDTH * charge_now / charge_full)
                                : WIDTH);

    // print summary of battery
    snprintf(buf, sizeof(buf), "/sys/class/power_supply/%s/status",
             battery_name);
    char status[64];
    sys_value_str_line(buf, status, sizeof(status));
    str_to_lower(status);
    snprintf(buf, sizeof(buf), "battery %u%%  %s",
             charge_full ? (uint32_t)(charge_now * 100 / charge_full) : 100,
             status);
    pl(buf);
}

static void render_acpi(void) {
    FILE* file = popen("acpi -at", "r");
    if (!file) {
        return;
    }
    uint32_t counter = RENDER_ACPI_MAX_LINE_COUNT;
    while (counter--) {
        char buf[512] = "";
        if (fscanf(file, "%511[^\n]%*c", buf) == EOF) {
            break;
        }
        str_to_lower(buf);
        pl(buf);
    }
    pclose(file);
}

static void render_bluetooth_connected_devices(void) {
    FILE* file = popen("bluetoothctl devices Connected | grep ^Device", "r");
    if (!file) {
        return;
    }
    char device_name[128] = "  ";
    uint32_t counter = RENDER_BLUETOOTH_CONNECTED_DEVICES_COUNT;
    while (counter--) {
        if (fscanf(file, "Device %*s %125[^\n]%*c", device_name + 2) == EOF) {
            // note: 125 and +2 because of the "  " prefix
            break;
        }
        if (counter == RENDER_BLUETOOTH_CONNECTED_DEVICES_COUNT - 1) {
            // note: -1 because counter starts at constant and decrements by 1
            //       at first iteration
            render_hr();
            pl("bluetooth devices connected:");
        }
        pl(device_name);
    }
    pclose(file);
}

static void render_syslog(void) {
    FILE* file = popen("journalctl -b -p6 -o cat -n 15 --no-pager", "r");
    if (!file) {
        return;
    }
    uint32_t counter = RENDER_SYSLOG_MAX_LINE_COUNT;
    while (counter--) {
        char buf[512] = "";
        if (fscanf(file, "%511[^\n]%*c", buf) == EOF) {
            break;
        }
        pl(buf);
    }
    pclose(file);
}

static void render_cheetsheet(void) {
    static char* keysheet[] = {"ĸey",
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
                               "+left     focus-previous",
                               "+shift+up        move-up",
                               "+shift+down    move-down",
                               "+0   i-am-bored-surprise",
                               " ...                ... ",
                               NULL};

    char** str_ptr = keysheet;
    while (*str_ptr) {
        pl(*str_ptr);
        str_ptr++;
    }
}

// static void render_top_10_processes(void) {
//   FILE *file = popen("ps -eo %mem,%cpu,comm --sort=-%mem | head -n 11",
//   "r"); if (!file) {
//     return;
//   }
//   char buf[512];
//   uint32_t counter = 11;
//   while (counter--) {
//     if (fscanf(file, "%511[^\n]%*c", buf) == EOF) {
//       break;
//     }
//     pl(buf);
//   }
//   pclose(file);
// }

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
    render_battery();
    render_acpi();
    render_bluetooth_connected_devices();
    render_hr();
    render_threads_throttle_visual();
    //    render_threads_throttle();
    render_hr();
    render_syslog();
    render_hr();
    render_hr();
    render_cheetsheet();
    render_hr();
    render_hr();
    dc_flush(dc);
}

static void signal_exit(int i) {
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

int main(int argc, char* argv[]) {
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

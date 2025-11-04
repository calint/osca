//
// reviewed: 2025-02-25
//

#ifndef CLONKY_CFG_H
#define CLONKY_CFG_H

// align left (0) or right (1) on screen
#define ALIGN 0

// start of clonky output
#define MARGIN_TOP 0

// width of clonky output
#define WIDTH 180

// font name and size
#define FONT_NAME "dejavu sans"
#define FONT_SIZE 8.0

// line height in pixels
#define LINE_HEIGHT 13

// pixels before and after horizontal rule
#define HR_PIXELS_BEFORE 4
#define HR_PIXELS_AFTER 0

// pixels of default graphs
#define DEFAULT_GRAPH_HEIGHT 35

// the y-values of the graph 1 MB
#define NET_GRAPH_MAX 1024 * 1024

// max number of network interfaces
#define NETIFC_ARRAY_SIZE 4

// size of network interface name
#define NETIFC_NAME_SIZE 16

// max number of connected bluetooth devices shown
#define RENDER_BLUETOOTH_CONNECTED_DEVICES_COUNT 8

// max number of lines read in 'render_syslog'
#define RENDER_SYSLOG_MAX_LINE_COUNT 15

// max number of lines read in 'render_df'
#define RENDER_DF_MAX_LINE_COUNT 64

// max number of lines read in 'render_acpi'
#define RENDER_ACPI_MAX_LINE_COUNT 64

#endif

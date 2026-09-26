#!/bin/sh
# enabling settings on touchpad
# 'xinput' to list devices
# 'xinput list-props [id]' to list properties
set -e

TOUCH_ID=$(xinput | grep -E "Touchpad|Synaptics" | awk -F'id=' '{print $2}' | awk '{print $1}')
echo Touchpad device ID: $TOUCH_ID

# look for "Tapping Enabled"
# xinput set-prop 9 325 1

PROP_ID=$(xinput list-props $TOUCH_ID | grep 'Tapping Enabled (' | grep -oP '\(\K\d+(?=\))')
echo "Set property Tapping Enabled ($PROP_ID) to 1"
xinput set-prop $TOUCH_ID $PROP_ID 1

# look for "Natural Scrolling Enabled"
# xinput set-prop 9 298 1

PROP_ID=$(xinput list-props $TOUCH_ID | grep 'Natural Scrolling Enabled (' | grep -oP '\(\K\d+(?=\))')
echo "Set property Natural Scrolling Enabled ($PROP_ID) to 1"
xinput set-prop $TOUCH_ID $PROP_ID 1

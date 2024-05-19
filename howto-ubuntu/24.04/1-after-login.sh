#!/bin/sh
# enabling settings on touchpad
# 'xinput' to list devices
# 'xinput list-props [id]' to list properties

TOUCH_ID=$(xinput | grep "ELAN1300:00 04F3:3104 Touchpad" | awk -F'id=' '{print $2}' | awk '{print $1}')
echo Touchpad device ID: $TOUCH_ID

# look for "Tapping Enabled"
# xinput set-prop 9 325 1

TAPPING_ENABLED=$(xinput list-props $TOUCH_ID | grep 'Tapping Enabled (' | grep -oP '\(\K\d+(?=\))')
for PROP_ID in $TAPPING_ENABLED; do
    echo "Set property Tapping Enabled ($PROP_ID) to 1"
    xinput set-prop $TOUCH_ID $PROP_ID 1
done

# look for "Natural Scrolling Enabled"
# xinput set-prop 9 298 1

SCROLL_ENABLED=$(xinput list-props $TOUCH_ID | grep 'Natural Scrolling Enabled (' | grep -oP '\(\K\d+(?=\))')
for PROP_ID in $SCROLL_ENABLED; do
    echo "Set property Natural Scrolling Enabled ($PROP_ID) to 1"
    xinput set-prop $TOUCH_ID $PROP_ID 1
done

# visual code
#   when pushing to github have the default browser open for the
#   interaction between visual code and github to work

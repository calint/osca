#!/bin/sh
# enabling settings on touchpad
# 'xinput' to list devices
# 'xinput list-props [id]' to list properties

# look for "Tapping Enabled"
xinput set-prop 9 325 1

# look for "Natural Scrolling Enabled"
xinput set-prop 9 298 1

# visual code
#   when pushing to github have the default browser open for the
#   interaction between visual code and github to work

import os
import sys

# Don't insert before . (first in list)
sys.path.insert(1, os.path.join('../gobject/.libs'))
sys.path.insert(1, os.path.join('../gtk/.libs'))

import gobject
import atk
import pango
import gtk
from gtk import gdk
from gtk import glade

import ltihooks
ltihooks.uninstall()

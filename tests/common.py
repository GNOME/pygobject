import os
import sys

# Don't insert before . (first in list)
sys.path.insert(1, os.path.join('..'))

path = os.path.abspath(os.path.join('..', 'gtk'))
import gobject
import atk
import pango
import gtk
from gtk import gdk
from gtk import glade

import ltihooks
ltihooks.uninstall()

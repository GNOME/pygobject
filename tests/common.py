import os
import sys

if not os.environ.has_key('DIST_CHECK'):
  sys.path.insert(0, '..')
  sys.path.insert(0, '../gobject')

import ltihooks

import gobject
import atk
import pango

import gtk
from gtk import gdk
try:
  from gtk import glade
except ImportError:
  glade = None

import testhelper

ltihooks.uninstall()

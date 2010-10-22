
"""pygobject is now installed on your machine.

Local configuration files were successfully updated."""

import os, re, sys

pkgconfig_file = os.path.normpath(
    os.path.join(sys.prefix,
                 'lib/pkgconfig/pygobject-2.0.pc'))

prefix_pattern=re.compile("^prefix=.*")
version_pattern=re.compile("Version: ([0-9]+\.[0-9]+\.[0-9]+)")

def replace_prefix(s):
    if prefix_pattern.match(s):
        s='prefix='+sys.prefix.replace("\\","/")+'\n'
    s=s.replace("@DATADIR@",
                os.path.join(sys.prefix,'share').replace("\\","/"))
    
    return s

def get_doc_url(pkgconfig_file, base_url):
    try:
        f = open(pkgconfig_file).read()
        ver = version_pattern.search(f).groups()[0]
        majv,minv,micv = ver.split('.')
        doc_url = "%s/%s.%s/" % (base_url,majv,minv)
    except:
        doc_url = base_url + "/stable/"
    return doc_url

# TODO : Check that shortcuts are created system-wide when the user
# has admin rights (hint: see pywin32 postinstall)
def create_shortcuts():
    progs_folder= get_special_folder_path("CSIDL_COMMON_PROGRAMS")
    site_packages_dir = os.path.join(sys.prefix , 'lib','site-packages')
   
    pygtk_shortcuts = os.path.join(progs_folder, 'PyGTK')
    if not os.path.isdir(pygtk_shortcuts):
        os.mkdir(pygtk_shortcuts)

    # link to specific documentation version by parsing the
    # pkgconfig file
    doc_url = get_doc_url(pkgconfig_file,
                          "http://library.gnome.org/devel/pygobject")
    pygobject_doc_link=os.path.join(pygtk_shortcuts,
                                    'PyGObject Documentation.lnk')
    if os.path.isfile(pygobject_doc_link):   
        os.remove(pygobject_doc_link)
    create_shortcut(doc_url,'PyGObject Documentation',pygobject_doc_link)
    file_created(pygobject_doc_link)

if len(sys.argv) == 2:
    if sys.argv[1] == "-install":
        # fixup the pkgconfig file
        lines=open(pkgconfig_file).readlines()
        open(pkgconfig_file, 'w').writelines(map(replace_prefix,lines))
        # TODO: Add an installer option for shortcut creation 
        # create_shortcuts()
        print __doc__


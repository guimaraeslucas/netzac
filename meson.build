project('zacos9-netzac', 'cpp',
  version: '4.0.8',
  license: 'GPL-3.0-or-later',
  default_options: ['cpp_std=c++17', 'warning_level=2'])

textdomain = get_option('textdomain')
localedir = get_option('prefix') / get_option('localedir')

qt6 = import('qt6')
i18n = import('i18n')

qt6_dep = dependency('qt6',
  modules: ['Core', 'Gui', 'Network', 'Widgets', 'WebEngineCore', 'WebEngineWidgets'])

subdir('shell/netzac')
subdir('po')

install_data('share/applications/zacos9-netzac.desktop',
  install_dir: get_option('datadir') / 'applications')

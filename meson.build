project('zacos9-netzac', 'cpp',
  version: '1.0.0',
  license: 'GPL-3.0-or-later',
  meson_version: '>=1.0',
  default_options: ['cpp_std=c++17', 'warning_level=2'])

textdomain = get_option('textdomain')
localedir = get_option('prefix') / get_option('localedir')

qt6 = import('qt6')
i18n = import('i18n')

qt6_dep = dependency('qt6',
  modules: ['Core', 'Gui', 'Network', 'Widgets', 'WebEngineCore', 'WebEngineWidgets'])

subdir('shell/netzac')
subdir('po')

install_data('share/applications/netzac.desktop',
  install_dir: get_option('datadir') / 'applications',
  install_tag: 'netzac')

install_data('share/icons/hicolor/scalable/apps/netzac.svg',
  install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
  install_tag: 'netzac')

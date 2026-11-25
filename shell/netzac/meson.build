moc_files = qt6.compile_moc(
  headers: ['adblocker.h', 'calmmode.h', 'widgets.h', 'dialogs.h', 'webview.h', 'netzacwindow.h'],
  dependencies: qt6_dep,
)

executable('zacos9-netzac',
  [
    'main.cpp',
    'adblocker.cpp',
    'bookmarks.cpp',
    'calmmode.cpp',
    'history.cpp',
    'platinum.cpp',
    'preferences.cpp',
    'widgets.cpp',
    'dialogs.cpp',
    'webview.cpp',
    'netzacwindow.cpp',
    moc_files,
  ],
  dependencies: [qt6_dep],
  cpp_args: [
    '-DZACOS9_TEXTDOMAIN="' + textdomain + '"',
    '-DZACOS9_LOCALEDIR_DEFAULT="' + localedir + '"',
  ],
  install: true,
  install_tag: 'netzac',
)

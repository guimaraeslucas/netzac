moc_files = qt6.compile_moc(
  headers: ['adblocker.h', 'calmmode.h', 'hibernation.h', 'preferences.h', 'widgets.h', 'dialogs.h', 'webview.h', 'netzacwindow.h'],
  dependencies: qt6_dep,
)

res_files = qt6.compile_resources(
  name: 'netzac_resources',
  sources: 'netzac.qrc',
)

netzac_cpp_args = [
  '-DZACOS9_TEXTDOMAIN="' + textdomain + '"',
  '-DZACOS9_LOCALEDIR_DEFAULT="' + localedir + '"',
]
netzac_link_args = []

if host_machine.system() == 'windows'
  # MSVC needs these for the Qt 6 headers and for UTF-8 sources (accented text).
  netzac_cpp_args += ['/utf-8', '/permissive-', '/Zc:__cplusplus']
  # Keep main() as entry point with win_subsystem: 'windows' without Qt6EntryPoint.
  netzac_link_args += ['/ENTRY:mainCRTStartup', 'advapi32.lib']
endif

netzac_sources = [
  'main.cpp',
  'adblocker.cpp',
  'bookmarks.cpp',
  'calmmode.cpp',
  'hibernation.cpp',
  'history.cpp',
  'platinum.cpp',
  'preferences.cpp',
  'widgets.cpp',
  'zacos9theme.cpp',
  'dialogs.cpp',
  'webview.cpp',
  'netzacwindow.cpp',
  moc_files,
  res_files,
]

if host_machine.system() == 'windows'
  netzac_icon_path = meson.project_source_root() / 'share' / 'icons' / 'netzac.ico'
  netzac_icon = files('../../share/icons/netzac.ico')

  # VERSIONINFO values. RC wants a comma separated 4 part version.
  version_parts = meson.project_version().split('.')
  if version_parts.length() < 3
    version_parts += ['0', '0']
  endif
  if version_parts.length() < 4
    version_parts += ['0']
  endif

  # 1. Icon in the .exe plus the VERSIONINFO block shown by Explorer
  #    (Properties > Details: product name, version, copyright).
  win = import('windows')
  netzac_rc = configure_file(
    input: 'netzac.rc.in',
    output: 'netzac.rc',
    configuration: {
      'ZACOS9_ICON_PATH': netzac_icon_path,
      'ZACOS9_VERSION': meson.project_version(),
      'ZACOS9_VERSION_QUAD': ','.join(version_parts),
      'ZACOS9_PRODUCT_NAME': product_name,
      'ZACOS9_PRODUCT_DESCRIPTION': product_description,
      'ZACOS9_COMPANY': product_company,
      'ZACOS9_COPYRIGHT': product_copyright,
    },
  )
  netzac_sources += win.compile_resources(
    netzac_rc,
    depend_files: netzac_icon,
  )

  # 2. Same icon in the Qt resource system, so setWindowIcon() works even when
  #    the executable is launched straight from the build tree.
  netzac_icon_qrc = configure_file(
    input: 'netzac-icon.qrc.in',
    output: 'netzac-icon.qrc',
    configuration: {
      'ZACOS9_ICON_PATH': netzac_icon_path,
    },
  )
  netzac_sources += qt6.compile_resources(
    name: 'netzac_icon_resources',
    sources: netzac_icon_qrc,
  )

  # 3. Translations: lrelease runs in this directory so the .qm files land next
  #    to the executable, which is where the application looks for them.
  fs = import('fs')
  lrelease = find_program('lrelease', required: true)
  linguas = fs.read(meson.project_source_root() / 'po' / 'LINGUAS')
  foreach lang : linguas.strip().split()
    custom_target('qm-' + lang,
      input: files(meson.project_source_root() / 'po' / (lang + '.po')),
      output: textdomain + '_' + lang + '.qm',
      command: [lrelease, '@INPUT@', '-qm', '@OUTPUT@'],
      build_by_default: true,
    )
  endforeach
endif

executable('zacos9-netzac',
  netzac_sources,
  dependencies: [qt6_dep],
  cpp_args: netzac_cpp_args,
  link_args: netzac_link_args,
  win_subsystem: 'windows',
  install: true,
  install_tag: 'netzac',
)

if host_machine.system() != 'windows'
  i18n.gettext(textdomain,
    args: [
      '--keyword=ztr:1',
      '--keyword=ztrn:1,2',
      '--from-code=UTF-8',
    ],
  )
endif
# On Windows there is no GNU gettext toolchain: the same catalogs are converted
# to Qt .qm files by lrelease in shell/netzac/meson.build, so they end up next
# to the executable where the application looks for them.

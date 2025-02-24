project('d1wm', 'cpp', 'c',
        version : '1.0.0',
        default_options : ['cpp_std=c++17'])

add_project_arguments('-Wmissing-field-initializers', '-Wunused-parameter',language : 'cpp')

wayland_scanner = find_program('wayland-scanner')
pkg_config = find_program('pkg-config')

wl_protocols_dir = run_command(pkg_config, '--variable=pkgdatadir', 'wayland-protocols').stdout().replace('\n', '')

protocol_sources = []
protocol_code = []

protocols = [
        wl_protocols_dir / 'stable' / 'xdg-shell' / 'xdg-shell.xml',
]

foreach protocol : protocols
        out_h = '@0@-protocol.h'.format(protocol)
        header = custom_target(
                out_h.underscorify() + '_h',
                input : protocol,
                output : '@BASENAME@-protocol.h',
                command : [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
        )
        protocol_sources += [header]

        out_c = '@0@-protocol.c'.format(protocol)
        code = custom_target(
                out_c.underscorify() + '_c',
                input : protocol,
                output : '@BASENAME@-protocol.c',
                command : [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
        )
        protocol_code += [code]
endforeach

include_dir = run_command(pkg_config, '--variable=includedir', 'wlroots-0.19').stdout().replace('\n', '')

include = include_directories(
        include_dir,
        include_dir / 'wlroots-0.19',
        'include',
)

libs = [
        dependency('wayland-server'),
        dependency('wayland-protocols'),
        dependency('wlroots-0.19'),
        dependency('pixman-1'),
        dependency('xkbcommon'),
        dependency('libinput'),
        dependency('xcb'),
]

executable(
        'd1wm',
        [
                'src/main.cpp',
                'src/server.cpp',
                'src/output.cpp',
                'src/cursor.cpp',
                'src/toplevel.cpp',
                'src/popup.cpp',
                protocol_sources,
                protocol_code,
        ],
        include_directories : include,
        dependencies : libs,
        install : true,
        install_dir : get_option('bindir'),
)
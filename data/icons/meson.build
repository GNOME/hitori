sizes = ['16x16', '22x22', '32x32', '48x48']

foreach size: sizes
    install_data(
        join_paths(size, application_id + '.png'),
        install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', size, 'apps')
    )
endforeach

install_data(
    join_paths('symbolic', application_id + '-symbolic.svg'),
    install_dir: join_paths(get_option('datadir'), 'icons', 'hicolor', 'symbolic', 'apps')
)
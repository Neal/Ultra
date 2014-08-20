jshint appinfo.json || exit $?
jshint src/js/pebble-js-app.js || exit $?
pebble build || exit $?
if [ "$1" = "install" ]; then
    pebble install --logs
fi

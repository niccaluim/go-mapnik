#!/bin/bash

cat > gen_import.go <<EOF
package mapnik
// #cgo CXXFLAGS: $(mapnik-config --cflags)
// #cgo LDFLAGS: $(mapnik-config --libs) -lboost_system
import "C"

const (
	fontPath = "$(mapnik-config --fonts)"
	pluginPath = "$(mapnik-config --input-plugins)"
)

EOF

if [ "$GO15VENDOREXPERIMENT" != "1" ]; then
    go install -x
fi

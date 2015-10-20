#!/bin/bash

cat > gen_import.go <<EOF
package mapnik
// #cgo CXXFLAGS: $(mapnik-config --cflags)
// #cgo LDFLAGS: $(mapnik-config --libs) $(mapnik-config --dep-libs | sed 's/-lpng|-lz//') -lpng -lz
import "C"

const (
	fontPath = "$(mapnik-config --fonts)"
	pluginPath = "$(mapnik-config --input-plugins)"
)

EOF

if [ "$GO15VENDOREXPERIMENT" != "1" ]; then
    go install -x
fi

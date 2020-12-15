#!/bin/bash
set -e

# Compile synth/synth.ts into build/synth/synth.js and dependencies
npx tsc -p tsconfig_synth_only.json

# Combine build/synth/synth.js and dependencies into website/beepbox_synth.js
npx rollup build/synth/synth.js \
	--file website/beepbox_synth.js \
	--format iife \
	--output.name beepbox \
	--context exports \
	--sourcemap \
	--plugin rollup-plugin-sourcemaps \
	--plugin @rollup/plugin-node-resolve

# Minify website/beepbox_synth.js into website/beepbox_synth.min.js
npx terser \
	website/beepbox_synth.js \
	--source-map "content='website/beepbox_synth.js.map',url=beepbox_synth.min.js.map" \
	-o website/beepbox_synth.min.js \
	--compress \
	--mangle \
	--mangle-props regex="/^_.+/;"

# Translate beepbox_synth.js to ES5, an older version of Javascript
npx google-closure-compiler \
	--js=website/beepbox_synth.js \
	--js_output_file=website/beepbox_synth.es5.js

# Combine build/synth/SynthBase.js and dependencies into website/beepbox_synthbase.js
npx rollup build/synth/SynthBase.js \
	--file website/beepbox_synthbase.js \
	--format iife \
	--output.name beepbox \
	--context exports \
	--sourcemap \
	--plugin rollup-plugin-sourcemaps \
	--plugin @rollup/plugin-node-resolve

# Minify website/beepbox_synthbase.js into website/beepbox_synthbase.min.js
npx terser \
	website/beepbox_synthbase.js \
	--source-map "content='website/beepbox_synthbase.js.map',url=beepbox_synthbase.min.js.map" \
	-o website/beepbox_synthbase.min.js \
	--compress \
	--mangle \
	--mangle-props regex="/^_.+/;"

# Translate beepbox_synthbase.js to ES5 (for MuJS compatibility)
npx google-closure-compiler \
	--js=website/beepbox_synthbase.js \
	--js_output_file=website/beepbox_synthbase.es5.js

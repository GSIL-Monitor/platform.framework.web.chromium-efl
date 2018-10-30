solutions = [{
  'managed'     : False,
  'name'        : 'src',
  'url'         : 'ssh://165.213.202.130:29418/webplatform/s-chromium.git',
  'custom_deps' : {
    'src/third_party/WebKit':None,
    'src/sbrowser':None,
    'src/third_party/skia':None,
    'src/v8':None,
    'src/third_party/libjingle/source/talk':None,
    'src/third_party/webrtc':None,
  },
  'deps_file'   : '.DEPS.git',
  'safesync_url': '',
  'custom_vars': {
    'webkit_rev': ''
  },
}]
cache_dir = None
target_os = ['linux']

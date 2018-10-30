{
  'target_defaults': {
    'conditions': [
      ['enable_s_android_browser==1', {
        'ldflags':[
          '-Wl,--gc-sections',
        ],
        'defines': [
          'S_NATIVE_SUPPORT=1',
        ],
      }],
      ['enable_s_dreamvr_browser==1', {
        'defines': ['S_DREAM_VR_SUPPORT=1'],
      }],
      ['enable_s_cowboy_browser==1', {
        'defines': ['S_TERRACE_SUPPORT=1'],
      }],
      ['OS=="android"', {
        'defines': ['ENABLE_MOBILE=1'],
      }],
      ['enable_kerberos_feature==1', {
        'defines': ['SBROWSER_KERBEROS_FEATURE=1']
      }],
    ],
  },
}

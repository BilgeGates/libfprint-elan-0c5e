/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <glib.h>
#include "elanmoc-legacy-storage.h"
int main(void) {
  guint cases=0;
  for(gsize length=0;length<257;length++)for(unsigned op=0;op<2;op++){
    uint8_t *uid=g_malloc0(MAX(length,1));memset(uid,'x',length);
    memcpy(uid,"FP1-",MIN(length,4));uint8_t frame[72];memset(frame,0xaa,72);
    int rc=elan_legacy_build_identity(op?0x13:0x11,uid,length,frame);
    gboolean good=length>=24&&length<=57;
    g_assert_cmpint(rc,==,good?0:-1);
    if(good){g_assert_cmpmem(frame,2,"\x40\xff",2);g_assert_cmpuint(frame[2],==,op?0x13:0x11);
      for(int i=3;i<7;i++)g_assert_cmpuint(frame[i],==,0);
      g_assert_cmpuint(frame[7],==,length);g_assert_cmpmem(frame+8,length,uid,length);
      for(gsize i=8+length;i<72;i++)g_assert_cmpuint(frame[i],==,0);
    }else for(int i=0;i<72;i++)g_assert_cmpuint(frame[i],==,0xaa);
    g_free(uid);cases++;
  }
  const uint8_t uid[]="FP1-20261027-7-12345678-test";
  for(unsigned op=0;op<256;op++){
    uint8_t frame[72];memset(frame,0xaa,72);
    int rc=elan_legacy_build_identity(op,uid,sizeof(uid)-1,frame);
    g_assert_cmpint(rc,==,(op==0x11||op==0x13)?0:-1);
    if(rc<0)for(int i=0;i<72;i++)g_assert_cmpuint(frame[i],==,0xaa);
    cases++;
  }
  for(gsize at=0;at<sizeof(uid)-1;at++){
    uint8_t broken[sizeof uid],frame[72];memcpy(broken,uid,sizeof uid);broken[at]=0;
    g_assert_cmpint(elan_legacy_build_identity(0x13,broken,sizeof(uid)-1,frame),==,-1);cases++;
  }
  g_print("PASS identity frame bounds/allowlist: %u cases\n",cases);
}

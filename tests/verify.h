// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "../kernels/common/default.h"
#include "../kernels/common/ray.h"
#include "../include/embree2/rtcore.h"
#include "../include/embree2/rtcore_ray.h"
#include "rtcore_helpers.h"
#include "../tutorials/common/tutorial/application.h"

namespace embree
{
  class VerifyApplication : public Application
  {
  public:
    enum TestType { PASS, FAIL, GROUP_BEGIN, GROUP_END };
    
    struct Test : public RefCount
    {
      Test (std::string name, TestType ty = PASS) 
        : name(name), ty(ty), enabled(false) {}

      virtual bool isEnabled(RTCDevice device) { return enabled; }
      virtual bool run(VerifyApplication* state) { return false; };

    public:
      std::string name;
      TestType ty;
      bool enabled;
    };

    struct IntersectTest : public Test
    {
      IntersectTest (std::string name, IntersectMode imode, IntersectVariant ivariant, TestType ty = PASS)
        : Test(name,ty), imode(imode), ivariant(ivariant) {}

      virtual bool isEnabled(RTCDevice device)
      { 
        switch (imode) {
        case MODE_INTERSECT_NONE: return Test::isEnabled(device);
        case MODE_INTERSECT1:   return Test::isEnabled(device) && rtcDeviceGetParameter1i(device,RTC_CONFIG_INTERSECT1);
        case MODE_INTERSECT4:   return Test::isEnabled(device) && rtcDeviceGetParameter1i(device,RTC_CONFIG_INTERSECT4);
        case MODE_INTERSECT8:   return Test::isEnabled(device) && rtcDeviceGetParameter1i(device,RTC_CONFIG_INTERSECT8);
        case MODE_INTERSECT16:  return Test::isEnabled(device) && rtcDeviceGetParameter1i(device,RTC_CONFIG_INTERSECT16);
        case MODE_INTERSECT1M:  return Test::isEnabled(device) && rtcDeviceGetParameter1i(device,RTC_CONFIG_INTERSECT_STREAM);
        case MODE_INTERSECTNM1: return Test::isEnabled(device) && rtcDeviceGetParameter1i(device,RTC_CONFIG_INTERSECT_STREAM);
        case MODE_INTERSECTNM3: return Test::isEnabled(device) && rtcDeviceGetParameter1i(device,RTC_CONFIG_INTERSECT_STREAM);
        case MODE_INTERSECTNM4: return Test::isEnabled(device) && rtcDeviceGetParameter1i(device,RTC_CONFIG_INTERSECT_STREAM);
        case MODE_INTERSECTNM8: return Test::isEnabled(device) && rtcDeviceGetParameter1i(device,RTC_CONFIG_INTERSECT_STREAM);
        case MODE_INTERSECTNM16:return Test::isEnabled(device) && rtcDeviceGetParameter1i(device,RTC_CONFIG_INTERSECT_STREAM);
        case MODE_INTERSECTNp:  return Test::isEnabled(device) && rtcDeviceGetParameter1i(device,RTC_CONFIG_INTERSECT_STREAM);
        }
        assert(false);
      }

    public:
      IntersectMode imode;
      IntersectVariant ivariant;
    };
    
  public:

    VerifyApplication ();

    void addTest(Ref<Test> test);
    void beginTestGroup(std::string name) { addTest(new Test(name,GROUP_BEGIN)); }
    void endTestGroup  () { addTest(new Test("",GROUP_END)); }
    bool runTest(Ref<Test> test, bool silent);
    void runTestGroup(size_t& id);

    int main(int argc, char** argv);
    
  public:
    RTCDevice device;
    float intensity;
    size_t numFailedTests;

  public:
    std::vector<Ref<Test>> tests;
    std::map<std::string,Ref<Test>> name2test;

  public:
    std::vector<RTCSceneFlags> sceneFlags;
    std::vector<RTCSceneFlags> sceneFlagsRobust;
    std::vector<RTCSceneFlags> sceneFlagsDynamic;
    std::vector<IntersectMode> intersectModes;
    std::vector<IntersectMode> intersectModesOld;
    std::vector<IntersectVariant> intersectVariants;
    bool user_specified_tests;
    bool use_groups;
  };
}

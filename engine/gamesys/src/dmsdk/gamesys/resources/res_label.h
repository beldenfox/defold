// Copyright 2020 The Defold Foundation
// Licensed under the Defold License version 1.0 (the "License"); you may not use
// this file except in compliance with the License.
//
// You may obtain a copy of the License, together with FAQs at
// https://www.defold.com/license
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#ifndef DMSDK_GAMESYS_RES_LABEL_H
#define DMSDK_GAMESYS_RES_LABEL_H

#include <gamesys/label_ddf.h>
#include <dmsdk/render/render.h>

namespace dmGameSystem
{
    struct LabelResource
    {
        dmGameSystemDDF::LabelDesc* m_DDF;
        dmRender::HMaterial m_Material;
        dmRender::HFontMap m_FontMap;
    };
}

#endif // DMSDK_GAMESYS_RES_LABEL_H

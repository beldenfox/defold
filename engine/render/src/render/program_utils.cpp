// Copyright 2020-2023 The Defold Foundation
// Copyright 2014-2020 King
// Copyright 2009-2014 Ragnar Svensson, Christian Murray
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

#include <dlib/log.h>
#include <dlib/dstrings.h>

#include "render.h"
#include "render_private.h"

namespace dmRender
{
    void GetProgramUniformCount(dmGraphics::HProgram program, uint32_t total_constants_count, uint32_t* constant_count_out, uint32_t* samplers_count_out)
    {
        uint32_t constants_count = 0;
        uint32_t samplers_count  = 0;
        int32_t value_count      = 0;

        dmGraphics::Type type;
        const uint32_t buffer_size = 128;
        char buffer[buffer_size];

        for (uint32_t i = 0; i < total_constants_count; ++i)
        {
            type = (dmGraphics::Type) -1;
            dmGraphics::GetUniformName(program, i, buffer, buffer_size, &type, &value_count);

            if (type == dmGraphics::TYPE_FLOAT_VEC4 || type == dmGraphics::TYPE_FLOAT_MAT4)
            {
                constants_count++;
            }
            else if (type == dmGraphics::TYPE_SAMPLER_2D || type == dmGraphics::TYPE_SAMPLER_CUBE || type == dmGraphics::TYPE_SAMPLER_2D_ARRAY)
            {
                samplers_count++;
            }
            else
            {
                dmLogWarning("Type for uniform %s is not supported (%d)", buffer, type);
            }
        }

        *constant_count_out = constants_count;
        *samplers_count_out = samplers_count;
    }

    void SetMaterialConstantValues(dmGraphics::HProgram program, uint32_t total_constants_count, dmHashTable64<dmGraphics::HUniformLocation>& name_hash_to_location, dmArray<RenderConstant>& constants, dmArray<Sampler>& samplers)
    {
        dmGraphics::Type type;
        const uint32_t buffer_size = 128;
        char buffer[buffer_size];
        int32_t num_values = 0;

        uint32_t default_values_capacity = 0;
        dmVMath::Vector4* default_values = 0;
        uint32_t sampler_index = 0;

        for (uint32_t i = 0; i < total_constants_count; ++i)
        {
            uint32_t name_str_length              = dmGraphics::GetUniformName(program, i, buffer, buffer_size, &type, &num_values);
            dmGraphics::HUniformLocation location = dmGraphics::GetUniformLocation(program, buffer);

            // DEF-2971-hotfix
            // Previously this check was an assert. In Emscripten 1.38.3 they made changes
            // to how uniforms are collected and reported back from WebGL. Simply speaking
            // in previous Emscripten versions you would get "valid" locations for uniforms
            // that wasn't used, but after the upgrade these unused uniforms will return -1
            // as location instead. The fix here is to avoid asserting on such values, but
            // not saving them in the m_Constants and m_NameHashToLocation structs.
            if (location == dmGraphics::INVALID_UNIFORM_LOCATION) {
                continue;
            }

            assert(name_str_length > 0);

            // For uniform arrays, OpenGL returns the name as "uniform[0]",
            // but we want to identify it as the base name instead.
            for (int j = 0; j < name_str_length; ++j)
            {
                if (buffer[j] == '[')
                {
                    buffer[j] = 0;
                    break;
                }
            }

            dmhash_t name_hash = dmHashString64(buffer);

            if (type == dmGraphics::TYPE_FLOAT_VEC4 || type == dmGraphics::TYPE_FLOAT_MAT4)
            {
                name_hash_to_location.Put(name_hash, location);

                HConstant render_constant = dmRender::NewConstant(name_hash);
                dmRender::SetConstantLocation(render_constant, location);

                if (type == dmGraphics::TYPE_FLOAT_MAT4)
                {
                    num_values *= 4;
                    dmRender::SetConstantType(render_constant, dmRenderDDF::MaterialDesc::CONSTANT_TYPE_USER_MATRIX4);
                }

                // Set correct size of the constant (Until the shader builder provides all the default values)
                if (num_values > default_values_capacity)
                {
                    default_values_capacity = num_values;
                    delete[] default_values;
                    default_values = new dmVMath::Vector4[default_values_capacity];
                    memset(default_values, 0, default_values_capacity * sizeof(dmVMath::Vector4));
                }
                dmRender::SetConstantValues(render_constant, default_values, num_values);

                RenderConstant constant;
                constant.m_Constant = render_constant;

                if (type == dmGraphics::TYPE_FLOAT_VEC4)
                {
                    size_t original_size = strlen(buffer);
                    dmStrlCat(buffer, ".x", sizeof(buffer));
                    constant.m_ElementIds[0] = dmHashString64(buffer);
                    buffer[original_size] = 0;
                    dmStrlCat(buffer, ".y", sizeof(buffer));
                    constant.m_ElementIds[1] = dmHashString64(buffer);
                    buffer[original_size] = 0;
                    dmStrlCat(buffer, ".z", sizeof(buffer));
                    constant.m_ElementIds[2] = dmHashString64(buffer);
                    buffer[original_size] = 0;
                    dmStrlCat(buffer, ".w", sizeof(buffer));
                    constant.m_ElementIds[3] = dmHashString64(buffer);
                    buffer[original_size] = 0;
                } else {
                    // Clear element ids, otherwise we will compare against
                    // uninitialized values in GetMaterialProgramConstantInfo.
                    constant.m_ElementIds[0] = 0;
                    constant.m_ElementIds[1] = 0;
                    constant.m_ElementIds[2] = 0;
                    constant.m_ElementIds[3] = 0;
                }
                constants.Push(constant);
            }
            else if (type == dmGraphics::TYPE_SAMPLER_2D || type == dmGraphics::TYPE_SAMPLER_CUBE || type == dmGraphics::TYPE_SAMPLER_2D_ARRAY)
            {
                name_hash_to_location.Put(name_hash, location);
                Sampler& s           = samplers[sampler_index];
                s.m_UnitValueCount   = num_values;

                switch(type)
                {
                    case dmGraphics::TYPE_SAMPLER_2D:
                        s.m_Type = dmGraphics::TEXTURE_TYPE_2D;
                        break;
                    case dmGraphics::TYPE_SAMPLER_2D_ARRAY:
                        s.m_Type = dmGraphics::TEXTURE_TYPE_2D_ARRAY;
                        break;
                    case dmGraphics::TYPE_SAMPLER_CUBE:
                        s.m_Type = dmGraphics::TEXTURE_TYPE_CUBE_MAP;
                        break;
                    default: assert(0);
                }
                sampler_index++;
            }
        }

        delete[] default_values;
    }
}

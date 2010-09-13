#include <stdio.h>
#include <graphics/glfw/glfw.h>
#include "../gui.h"

#ifdef __MACH__
#include <Carbon/Carbon.h>
#endif

GLuint checker_texture;

void MyRenderNode(dmGui::HScene scene,
                  const dmGui::Node* nodes,
                  uint32_t node_count,
                  void* context)
{
    for (uint32_t i = 0; i < node_count; ++i)
    {
        const dmGui::Node* node = &nodes[i];

        Vector4 pos = node->m_Properties[dmGui::PROPERTY_POSITION];
        Vector4 rot = node->m_Properties[dmGui::PROPERTY_ROTATION];
        Vector4 ext = node->m_Properties[dmGui::PROPERTY_EXTENTS];
        Vector4 color = node->m_Properties[dmGui::PROPERTY_COLOR];
        glColor4f(color.getX(), color.getY(), color.getZ(), color.getW());


        dmGui::BlendMode blend_mode = (dmGui::BlendMode) node->m_BlendMode;
        switch (blend_mode)
        {
            case dmGui::BLEND_MODE_ALPHA:
                glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            break;

            case dmGui::BLEND_MODE_ADD:
                glBlendFunc(GL_ONE, GL_ONE);
            break;

            case dmGui::BLEND_MODE_ADD_ALPHA:
                glBlendFunc(GL_ONE, GL_SRC_ALPHA);
                break;

            case dmGui::BLEND_MODE_MULT:
                glBlendFunc(GL_ZERO, GL_SRC_COLOR);
                break;

            default:
                printf("Unknown blend mode: %d\n", blend_mode);
                break;
        }


        if (node->m_Texture)
        {
            glEnable(GL_TEXTURE_2D);
            glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
            glBindTexture(GL_TEXTURE_2D, checker_texture);
        }
        else
        {
            glDisable(GL_TEXTURE_2D);
        }

        glPushMatrix();

        float x = pos.getX();
        float y = pos.getY();
        float z = pos.getZ();

        glTranslatef(x, y, z);
        glRotatef(rot.getX(), 1, 0, 0);
        glRotatef(rot.getY(), 0, 1, 0);
        glRotatef(rot.getZ(), 0, 0, 1);
        glTranslatef(-x, -y, -z);

        float dx = ext.getX() * 0.5f;
        float dy = ext.getY() * 0.5f;
        glBegin(GL_QUADS);

        glTexCoord2f(0, 0);
        glVertex3f(x - dx, y - dy, z);
        glTexCoord2f(1, 0);
        glVertex3f(x + dx, y - dy, z);
        glTexCoord2f(1, 1);
        glVertex3f(x + dx, y + dy, z);
        glTexCoord2f(0, 1);
        glVertex3f(x - dx, y + dy, z);

        glEnd();

        glPopMatrix();
    }
}

int main(void)
{
    int width, height, running, x, y;
    float t;

    uint8_t checker_texture_data[] = {128, 128, 128, 255,
                                      255, 255, 255, 255,
                                      255, 255, 255, 255,
                                      128, 128, 128, 255};

    glfwInit();

    if (!glfwOpenWindow(700, 700, 8, 8, 8, 8, 0, 0, GLFW_WINDOW))
    {
        glfwTerminate();
        return 1;
    }

    glfwEnable(GLFW_STICKY_KEYS);
    glfwSwapInterval(1);

    dmGui::HGui gui = dmGui::New();
    dmGui::NewSceneParams params;
    params.m_MaxNodes = 256;
    params.m_MaxAnimations = 1024;
    dmGui::HScene scene = dmGui::NewScene(gui, &params);

    glGenTextures(1, &checker_texture);
    glBindTexture(GL_TEXTURE_2D, checker_texture);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, &checker_texture_data[0]);

    const char* script_file = "src/test/guidemo.lua";
    FILE*f = fopen(script_file, "rb");
    if (!f)
    {
        printf("Unable to open: %s\n", script_file);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    uint32_t file_size = (uint32_t) ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = new char[file_size];
    fread(buf, 1, file_size, f);
    fclose(f);

    dmGui::AddTexture(scene, "checker", (void*) checker_texture);
    dmGui::SetSceneScript(scene, buf, file_size);

#ifdef __MACH__
    ProcessSerialNumber psn;
    OSErr err;

    // Move window to front. Required if running without application bundle.
    err = GetCurrentProcess( &psn );
    if (err == noErr)
        (void) SetFrontProcess( &psn );
#endif
    glfwSetWindowTitle("GUI Demo");

    running = GL_TRUE;
    while (running)
    {

        dmGui::UpdateScene(scene, 1.0f / 60.0f);

        t = glfwGetTime();
        glfwGetMousePos(&x, &y);

        glfwGetWindowSize(&width, &height);
        height = height > 0 ? height : 1;

        glViewport(0, 0, width, height);

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear( GL_COLOR_BUFFER_BIT);

        glEnable (GL_BLEND);

        glMatrixMode( GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, 1, 0, 1);

        glMatrixMode( GL_MODELVIEW);
        glLoadIdentity();

        dmGui::RenderScene(scene, &MyRenderNode, 0);

        glfwSwapBuffers();

        running = !glfwGetKey(GLFW_KEY_ESC) && glfwGetWindowParam(GLFW_OPENED);
    }

    glfwTerminate();

    return 0;
}

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "vulkan/vulkan.h"

#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"
#include "stdbool.h"
#include "string.h"
#include "stdalign.h"
#include "time.h"
#include "float.h"

#include "file_helpers.h"
#include "linal.h"
#include "linal_quat.h"
#include "lava.h"
#include "raw_vertices_reader.h"
#include "misc.h"
#include "functions.h"
#include "camera.h"


#ifndef RELEASE_MODE
const char *validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
};
#else
const char *validation_layers[] = {
};
#endif
const size_t validation_layer_count = sizeof validation_layers / sizeof(char*);


// Inputs
static vec2 last_mouse_position;
static bool is_mouse_clicked;
// Game state
static clock_t start_time;
static Camera camera;
// GLFW
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
static GLFWwindow *window;
// Vulkan
static Vulkan vulkan;
// Drawing
static Model model;


void gameUpdateModelDirection() {
    if (!is_mouse_clicked) {
        return;
    }

#if 0 /* Faster(?) version of controls, way too junky though*/

    /* Should be computed only once */
    quaternion min_x = quat_from_angle_axis(0.0, (vec3){1, 0, 0});
    quaternion max_x = quat_from_angle_axis(0.5, (vec3){1, 0, 0});

    quaternion min_y = quat_from_angle_axis(0.0, (vec3){0, 1, 0});
    quaternion max_y = quat_from_angle_axis(0.5, (vec3){0, 1, 0});

   
    double x;
    double y;
    
    glfwGetCursorPos(window, &x, &y); 
    float tx = -(y - last_mouse_position.y) / (float)WINDOW_HEIGHT;
    float ty = (x - last_mouse_position.x) / (float)WINDOW_WIDTH;

    quaternion x_diff = quat_lerp(min_x, max_x, tx);
    quaternion y_diff = quat_lerp(min_y, max_y, ty);

    model.orientation = quat_mult(model.orientation, x_diff);
    model.orientation = quat_mult(model.orientation, y_diff);
    
    last_mouse_position.x = (float) x;
    last_mouse_position.y = (float) y;
#endif 

#if 1
    const float rotations_per_screen = 0.5;

    double x;
    double y;
    
    glfwGetCursorPos(window, &x, &y); 
    float angle_x = -(y - last_mouse_position.y) / (float)WINDOW_HEIGHT * rotations_per_screen;
    float angle_y = (x - last_mouse_position.x) / (float)WINDOW_WIDTH * rotations_per_screen;
    
    quaternion x_diff = quat_from_angle_axis(angle_x, (vec3){1, 0, 0});
    quaternion y_diff = quat_from_angle_axis(angle_y, (vec3){0, 1, 0});

    model.orientation = quat_mult(model.orientation, x_diff);
    model.orientation = quat_mult(model.orientation, y_diff);
    
    last_mouse_position.x = (float) x;
    last_mouse_position.y = (float) y;
#endif
}

#if 0 
void gameUpdateCameraPosition() { 
    quaternion pos_diff = quat_conjugate(camera.direction);
    camera.position = vec_rotate_by_quat((vec3){0, 0, camera.distance}, pos_diff); 
}

#endif

void gameUpdateCameraDirection() {
    /* in our world x and y are swaped*/
    
    if (!is_mouse_clicked) {
        return;
    }
    const float rotations_per_screen = 0.5;

    double x;
    double y;
    
    glfwGetCursorPos(window, &x, &y); 
    float angle_x = -(y - last_mouse_position.y) / (float)WINDOW_HEIGHT * rotations_per_screen;
    float angle_y = -(x - last_mouse_position.x) / (float)WINDOW_WIDTH * rotations_per_screen;

    fps_camera_rotate(&camera, angle_x, angle_y);

    last_mouse_position.x = (float) x;
    last_mouse_position.y = (float) y;
}

/* @SPEED: view and projection matrices are recomputed on each frame */
void gameUpdateUniformBuffer(UniformBuffer uniform, clock_t game_start) {
#if 0 /* currently unused */
    clock_t now = clock();

    float diff_seconds = (now - game_start) / (float)CLOCKS_PER_SEC;
#endif
    Ubo ubo;

    ubo.model = quat_to_mat4t(model.orientation);
    ubo.model.m[3][0] = model.position.x;
    ubo.model.m[3][1] = model.position.y;
    ubo.model.m[3][2] = model.position.z;

    mat4t view_inv = quat_to_mat4t(camera.direction);
    view_inv.m[3][0] = camera.position.x;
    view_inv.m[3][1] = camera.position.y;
    view_inv.m[3][2] = camera.position.z; 
    ubo.view = mat4t_inverse(view_inv);

#if 0
    ubo.view = quat_to_mat4t(camera.direction);
    ubo.view.m[3][0] = camera.position.x;
    ubo.view.m[3][1] = camera.position.y;
    ubo.view.m[3][2] = camera.position.z; 
#endif

    ubo.projection = linal_mat4t_frustum(15.0f, 1000.0f, -15.0f, 15.0f, -15.0f, 15.0f);
    memcpy(uniform.mapped_memory, &ubo, sizeof(Ubo)); 
}


// @TODO: I should compose SyncObjects with something else
VkResult gameDrawFrame(Vulkan *vulkan, clock_t start_time) {
    vkWaitForFences(vulkan->device, 1, &vulkan->sync.inFlight, VK_TRUE, UINT64_MAX);
    VkResult res;

    uint32_t image_index;
    res = vkAcquireNextImageKHR(vulkan->device, vulkan->swapchain.swapchain, UINT64_MAX, vulkan->sync.imageAvailable, VK_NULL_HANDLE, &image_index);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        return res; 
    } 

    vkResetFences(vulkan->device, 1, &vulkan->sync.inFlight);
 
    /* game state apdate */ {
        gameUpdateCameraDirection();
        //gameUpdateCameraPosition();
        gameUpdateUniformBuffer(vulkan->uniform_buffer, start_time);
    }

    vkResetCommandBuffer(vulkan->command_buffer, 0);
    vulkanRecordCommandBuffer(vulkan->command_buffer, vulkan->swapchain, vulkan->pipeline, vulkan->pipeline_layout.layout, vulkan->vertex_buffer, vulkan->uniform_buffer, image_index, model.vertices_len);

    
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info = {0};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &vulkan->sync.imageAvailable;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vulkan->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &vulkan->sync.renderFinished;

    // Probably should return here instead of failing? I don't know API well enough
    VK_CHECK(vkQueueSubmit(vulkan->graphics_queue, 1, &submit_info, vulkan->sync.inFlight));
    
    VkPresentInfoKHR present_info = {0};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &vulkan->sync.renderFinished;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &vulkan->swapchain.swapchain;
    present_info.pImageIndices = &image_index;
    present_info.pResults = NULL;

    return vkQueuePresentKHR(vulkan->present_queue, &present_info);
}


// @TODO: make smooth window resizing, as for now it is terrible
static void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    (void)window;
    (void)width;
    (void)height;
    /*
    vkDeviceWaitIdle(device);

    freeSwapchain(device, swapchain);
    
    swapchain = vulkanInitSwapchain(gpu,device, surface, window); 
    vulkanSwapchainCreateRenderPass(device, &swapchain);
    vulkanSwapchainCreateFramebuffers(device, &swapchain); 
    */
    
    gameDrawFrame(
        &vulkan, 
        start_time
    );

    /*
    if (res != VK_SUCCESS) {
        fprintf(stderr, "Failed to draw frame after resize");
        exit(1);
    }
    */
}

void keyCallback(
    GLFWwindow *window,
    int key,
    int scancode,
    int action,
    int mods
) {
    (void) window;
    (void) mods;
    (void) scancode;

    // unhardcode camera controls, maybe?
    
    if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        //@tmp: testing
        //camera.distance += 5.0f;
        //gameUpdateCameraPosition();
        //
        camera.position.z += 5.0;
        return;
    }

    if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT)) {
        //@tmp
        //camera.distance -= 5.0f;
        //gameUpdateCameraPosition();
        //
        camera.position.z -= 5.0;
        return;
    }
    
    fprintf(stderr, "GLFW: Action(%d) with Key(%d)\n", action, key);
}

void mouseButtonCallback(
    GLFWwindow *window,
    int button,
    int action,
    int mods
) {
    (void) window;
    (void) mods;

    fprintf(stderr, "GLFW: MouseButton(%d), Action(%d)\n", button, action);
    if (button == GLFW_MOUSE_BUTTON_LEFT) { 
        double x;
        double y;
        glfwGetCursorPos(window, &x, &y);

        last_mouse_position.x = (float) x;
        last_mouse_position.y = (float) y;
        is_mouse_clicked = !is_mouse_clicked;
    }
}

int main() {
    // @LEAK
    /* reading teapot data */ {
        vec3 *raw_teapot_vert = readVerticesFromFile("assets/teapot_bezier2.tris", &model.vertices_len);
        if (raw_teapot_vert == NULL) {
            fprintf(stderr, "ERROR: failed to load teapot vertices\n");
            exit(1);
        }

        /* some scaling */
        for (size_t i=0;i<model.vertices_len;++i) {
            raw_teapot_vert[i] = vec3_scale(raw_teapot_vert[i], 10);
            raw_teapot_vert[i].y = -raw_teapot_vert[i].y;
        }

        /* centering the model */
        float miny = FLT_MAX;
        float maxy = 0.0;

        for (size_t i=0;i<model.vertices_len;++i) {
            vec3 v = raw_teapot_vert[i];
            miny = v.y < miny ? v.y : miny; 
            maxy = v.y > maxy ? v.y : maxy;
        }

        vec3 mids = {0, (maxy - miny)/2.0, 0};
    
        model.vertices = malloc(model.vertices_len * sizeof(Vertex));
        if (model.vertices == NULL) {
            fprintf(stderr, "ERROR: failed to allocate memory for teapot vertices");
            exit(1);
        }

        for (size_t i=0; i<model.vertices_len; ++i) {
            if (i % 9 <= 2) {
                model.vertices[i] = (Vertex){vec3_add(raw_teapot_vert[i], mids), (vec4){1, 1, 1, 1}};
            } else if (i % 9 <= 5) {
                model.vertices[i] = (Vertex){vec3_add(raw_teapot_vert[i], mids), (vec4){1, 1, 0, 1}}; 
            } else {
                model.vertices[i] = (Vertex){vec3_add(raw_teapot_vert[i], mids), (vec4){0, 0, 1, 1}};
            }
        } 

        free(raw_teapot_vert);
    }

    /* App state init */ {
        start_time = clock(); 

        model.orientation = (quaternion) {0.0, 0.0, 0.0, 0.0}; /*quat_from_angle_axis(0.125, (vec3){0.0, 0.0, 1.0}); */
        model.position = (vec3){0.0, 0.0, 0.0};

        /*
        camera.direction = quat_from_angle_axis(0.0, (vec3){0.0, 0.0, 1.0});
        camera.position = VEC3(0, 0, -100);
        */

        camera.position = VEC3(0, 0, 0);
        camera.direction = quat_from_angle_axis(0.1, (vec3){0.1, 0.0, 0.0});
    }

    /* GLFW init */ {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        // is this the way to go?
        window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Xeno", NULL, NULL);
        
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        if (glfwRawMouseMotionSupported()) {
            glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
        }

        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
    }

    /* Vulkan init */ {
        vulkan = vulkanCompleteInit(window, validation_layers, validation_layer_count, "shaders_out/vert.spv", "shaders_out/frag.spv");
        vulkanCreateVertexBuffer(&vulkan, model.vertices, model.vertices_len * sizeof(Vertex));
    }
    
    gameUpdateUniformBuffer(vulkan.uniform_buffer, start_time);
    while(!glfwWindowShouldClose(window)) {
        glfwPollEvents(); 
        VkResult res = gameDrawFrame(
            &vulkan, 
            start_time
        );

        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
            vulkanRecreateSwapchain(&vulkan, window);
            continue;
        }
        if (res != VK_SUCCESS) {
            fprintf(stderr, "Failed to accuire next image. Error code: %d", res);
            exit(1);
        }
    }
    
    vulkanFree(&vulkan);
    glfwDestroyWindow(window);
    glfwTerminate(); 
}

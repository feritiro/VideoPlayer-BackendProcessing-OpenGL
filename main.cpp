#include <GL/glew.h>
#include <GLFW/glfw3.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include <iostream>
#include <cstdio>

// Force NVIDIA GPU if available (optional, for dual-GPU systems)
//extern "C" {
//    __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
//}

GLuint CreateTexture(int width, int height) {
    GLuint texID;
    glGenTextures(1, &texID);
    if (texID == 0) {
        std::cerr << "Erro: Falha ao gerar textura\n";
        return 0;
    }
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        std::cerr << "Erro OpenGL ao criar textura: " << glErr << std::endl;
        glDeleteTextures(1, &texID);
        return 0;
    }
    return texID;
}

GLuint CreateSimpleShader() {
    const char* vs = R"(#version 330
    layout(location = 0) in vec2 pos;
    layout(location = 1) in vec2 texCoord;
    out vec2 TexCoord;
    void main() {
        TexCoord = texCoord;
        gl_Position = vec4(pos, 0.0, 1.0);
    })";

    const char* fs = R"(#version 330
    in vec2 TexCoord;
    out vec4 FragColor;
    uniform sampler2D tex;
    void main() {
        FragColor = texture(tex, TexCoord);
    })";

    auto compile = [](GLenum type, const char* src) -> GLuint {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);
        GLint success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[512];
            glGetShaderInfoLog(shader, 512, nullptr, log);
            std::cerr << "Erro ao compilar shader: " << log << std::endl;
            glDeleteShader(shader);
            return 0;
        }
        return shader;
        };

    GLuint vsID = compile(GL_VERTEX_SHADER, vs);
    if (vsID == 0) return 0;
    GLuint fsID = compile(GL_FRAGMENT_SHADER, fs);
    if (fsID == 0) {
        glDeleteShader(vsID);
        return 0;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vsID);
    glAttachShader(program, fsID);
    glLinkProgram(program);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cerr << "Erro ao linkar shader: " << log << std::endl;
        glDeleteShader(vsID);
        glDeleteShader(fsID);
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vsID);
    glDeleteShader(fsID);
    return program;
}

const char* videoPath = "C:\\Users\\ferni\\Videos\\Captures\\video.mp4";

int main() {
    // Verify video file exists
    FILE* testFile = nullptr;
    errno_t err = fopen_s(&testFile, videoPath, "rb");
    if (err != 0 || !testFile) {
        std::cerr << "Erro: Não foi possível abrir o arquivo de vídeo: " << videoPath << std::endl;
        return -1;
    }
    fclose(testFile);

    // Initialize FFmpeg
    avformat_network_init();
    AVFormatContext* formatCtx = nullptr;
    if (avformat_open_input(&formatCtx, videoPath, nullptr, nullptr) != 0) {
        std::cerr << "Erro ao abrir o vídeo\n";
        return -1;
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        std::cerr << "Erro ao encontrar informações do fluxo\n";
        avformat_close_input(&formatCtx);
        return -1;
    }

    int videoStream = -1;
    for (unsigned i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    if (videoStream == -1) {
        std::cerr << "Stream de vídeo não encontrado\n";
        avformat_close_input(&formatCtx);
        return -1;
    }

    AVCodecParameters* codecPar = formatCtx->streams[videoStream]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        std::cerr << "Codec não encontrado\n";
        avformat_close_input(&formatCtx);
        return -1;
    }

    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        std::cerr << "Erro ao alocar contexto do codec\n";
        avformat_close_input(&formatCtx);
        return -1;
    }

    if (avcodec_parameters_to_context(codecCtx, codecPar) < 0) {
        std::cerr << "Erro ao copiar parâmetros do codec\n";
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        std::cerr << "Erro ao abrir o codec\n";
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    // Calculate frame rate for timing
    double frameDelay = 1.0;
    if (formatCtx->streams[videoStream]->avg_frame_rate.den != 0) {
        frameDelay = (double)formatCtx->streams[videoStream]->avg_frame_rate.den /
            formatCtx->streams[videoStream]->avg_frame_rate.num;
    }
    std::cout << "Frame delay: " << frameDelay << " seconds\n";

    // Ensure even dimensions for scaling
    int width = codecCtx->width;
    int height = codecCtx->height;
    if (width % 2 != 0) width++;
    if (height % 2 != 0) height++;
    std::cout << "Resolution: " << width << "x" << height << std::endl;

    SwsContext* swsCtx = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
        width, height, AV_PIX_FMT_RGB24, SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!swsCtx) {
        std::cerr << "Erro ao criar contexto de escala\n";
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Erro ao alocar frame\n";
        sws_freeContext(swsCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    AVFrame* rgbFrame = av_frame_alloc();
    if (!rgbFrame) {
        std::cerr << "Erro ao alocar rgbFrame\n";
        av_frame_free(&frame);
        sws_freeContext(swsCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    if (numBytes < 0) {
        std::cerr << "Erro ao calcular tamanho do buffer\n";
        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    uint8_t* buffer = (uint8_t*)av_malloc(numBytes);
    if (!buffer) {
        std::cerr << "Erro ao alocar buffer RGB\n";
        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    if (av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, buffer,
        AV_PIX_FMT_RGB24, width, height, 1) < 0) {
        std::cerr << "Erro ao preencher arrays RGB\n";
        av_free(buffer);
        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    // Initialize GLFW and OpenGL
    if (!glfwInit()) {
        std::cerr << "Erro ao inicializar GLFW\n";
        av_free(buffer);
        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(width, height, "Video Player", nullptr, nullptr);
    if (!window) {
        std::cerr << "Erro ao criar janela GLFW\n";
        glfwTerminate();
        av_free(buffer);
        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK) {
        std::cerr << "Erro ao inicializar GLEW\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        av_free(buffer);
        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    GLuint texID = CreateTexture(width, height);
    if (texID == 0) {
        std::cerr << "Erro ao criar textura\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        av_free(buffer);
        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    GLuint shaderID = CreateSimpleShader();
    if (shaderID == 0) {
        std::cerr << "Erro ao criar shader\n";
        glDeleteTextures(1, &texID);
        glfwDestroyWindow(window);
        glfwTerminate();
        av_free(buffer);
        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    glUseProgram(shaderID);
    glUniform1i(glGetUniformLocation(shaderID, "tex"), 0);
    GLenum glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        std::cerr << "Erro OpenGL ao configurar uniform: " << glErr << std::endl;
        glDeleteProgram(shaderID);
        glDeleteTextures(1, &texID);
        glfwDestroyWindow(window);
        glfwTerminate();
        av_free(buffer);
        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    float quad[] = {
        -1,  1, 0.0, 0.0,
        -1, -1, 0.0, 1.0,
         1, -1, 1.0, 1.0,
         1,  1, 1.0, 0.0,
    };
    GLuint indices[] = { 0, 1, 2, 2, 3, 0 };
    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glErr = glGetError();
    if (glErr != GL_NO_ERROR) {
        std::cerr << "Erro OpenGL ao configurar VAO/VBO/EBO: " << glErr << std::endl;
        glDeleteBuffers(1, &ebo);
        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
        glDeleteProgram(shaderID);
        glDeleteTextures(1, &texID);
        glfwDestroyWindow(window);
        glfwTerminate();
        av_free(buffer);
        av_frame_free(&frame);
        av_frame_free(&rgbFrame);
        sws_freeContext(swsCtx);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    // Main loop
    AVPacket pkt = { 0 };
    double lastFrameTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        // Read and decode frames
        bool newFrame = false;
        while (av_read_frame(formatCtx, &pkt) >= 0) {
            if (pkt.stream_index == videoStream) {
                if (avcodec_send_packet(codecCtx, &pkt) < 0) {
                    std::cerr << "Erro ao enviar pacote\n";
                    av_packet_unref(&pkt);
                    continue;
                }
                int ret = avcodec_receive_frame(codecCtx, frame);
                if (ret == 0 && frame->data[0]) {
                    sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height,
                        rgbFrame->data, rgbFrame->linesize);
                    if (rgbFrame->data[0]) {
                        newFrame = true;
                    }
                    else {
                        std::cerr << "Erro: rgbFrame->data[0] é nulo após sws_scale\n";
                    }
                    av_packet_unref(&pkt);
                    break; // Got a frame, render it
                }
                else if (ret < 0 && ret != AVERROR(EAGAIN)) {
                    char err_buf[128];
                    av_make_error_string(err_buf, 128, ret);
                    std::cerr << "Erro ao receber frame: " << err_buf << std::endl;
                }
                av_packet_unref(&pkt);
            }
            av_packet_unref(&pkt);
        }

        if (!newFrame && av_read_frame(formatCtx, &pkt) < 0) {
            std::cout << "Fim do vídeo\n";
            break; // End of video
        }

        // Update texture and render
        if (newFrame) {
            glBindTexture(GL_TEXTURE_2D, texID);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, rgbFrame->data[0]);
            glErr = glGetError();
            if (glErr != GL_NO_ERROR) {
                std::cerr << "Erro OpenGL ao atualizar textura: " << glErr << std::endl;
                break;
            }
        }

        // Render frame
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderID);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glErr = glGetError();
        if (glErr != GL_NO_ERROR) {
            std::cerr << "Erro OpenGL ao renderizar: " << glErr << std::endl;
            break;
        }
        glfwSwapBuffers(window);

        // Control frame rate
        double currentTime = glfwGetTime();
        while (currentTime - lastFrameTime < frameDelay) {
            currentTime = glfwGetTime();
        }
        lastFrameTime = currentTime;

        glfwPollEvents();
    }

    // Cleanup
    av_packet_unref(&pkt);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderID);
    glDeleteTextures(1, &texID);
    glfwDestroyWindow(window);
    glfwTerminate();
    av_free(buffer);
    sws_freeContext(swsCtx);
    av_frame_free(&frame);
    av_frame_free(&rgbFrame);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);
    return 0;
}
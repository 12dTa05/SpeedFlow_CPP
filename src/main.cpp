#include <iostream>
#include <csignal>
#include <glib.h>
#include "pipeline_builder.h"
#include "config_loader.h"

static GMainLoop* g_main_loop = nullptr;
static PipelineBuilder* g_pipeline = nullptr;

void signalHandler(int signum) {
    std::cout << "\n[Main] Interrupt signal (" << signum << ") received. Shutting down..." << std::endl;
    
    if (g_pipeline) {
        g_pipeline->stop();
    }
    
    if (g_main_loop) {
        g_main_loop_quit(g_main_loop);
    }
}

void printUsage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " <source_uri> [options]\n"
              << "\nArguments:\n"
              << "  source_uri          RTSP URI (rtsp://...) or file path (file:///...)\n"
              << "\nOptions:\n"
              << "  --config <path>     Path to pipeline config YAML (default: configs/pipeline.yml)\n"
              << "  --help              Show this help message\n"
              << "\nExamples:\n"
              << "  " << prog_name << " rtsp://192.168.1.100/stream\n"
              << "  " << prog_name << " file:///path/to/video.mp4 --config my_config.yml\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse arguments
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string source_uri = argv[1];
    std::string config_path = "configs/pipeline.yml";
    
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        }
    }
    
    std::cout << "==================================================" << std::endl;
    std::cout << "  SpeedFlow C++ - Traffic Monitoring System" << std::endl;
    std::cout << "  Version 1.0.0" << std::endl;
    std::cout << "==================================================" << std::endl;
    std::cout << "Source URI: " << source_uri << std::endl;
    std::cout << "Config: " << config_path << std::endl;
    std::cout << "==================================================" << std::endl;
    
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Load configuration
        std::cout << "[Main] Loading configuration..." << std::endl;
        PipelineConfig config = ConfigLoader::loadPipelineConfig(config_path);
        
        // Build pipeline
        std::cout << "[Main] Building pipeline..." << std::endl;
        g_pipeline = new PipelineBuilder(config);
        
        if (!g_pipeline->build(source_uri)) {
            std::cerr << "[Main] Failed to build pipeline" << std::endl;
            delete g_pipeline;
            return 1;
        }
        
        // Start pipeline
        std::cout << "[Main] Starting pipeline..." << std::endl;
        if (!g_pipeline->start()) {
            std::cerr << "[Main] Failed to start pipeline" << std::endl;
            delete g_pipeline;
            return 1;
        }
        
        // Run main loop
        std::cout << "[Main] Running... (Press Ctrl+C to stop)" << std::endl;
        g_main_loop = g_main_loop_new(nullptr, FALSE);
        g_main_loop_run(g_main_loop);
        
        // Cleanup
        std::cout << "[Main] Cleaning up..." << std::endl;
        g_main_loop_unref(g_main_loop);
        delete g_pipeline;
        
        std::cout << "[Main] Shutdown complete" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[Main] Fatal error: " << e.what() << std::endl;
        if (g_pipeline) delete g_pipeline;
        return 1;
    }
    
    return 0;
}

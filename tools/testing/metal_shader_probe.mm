#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include <cstdio>
#include <cstdint>
#include <cstring>

int main(int argc, char** argv) {
 @autoreleasepool {
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  if (!device) { fprintf(stderr, "No Metal device available\n"); return 2; }
  fprintf(stderr, "Metal shader probe device: %s\n", device.name.UTF8String);
  int failures = 0;
  int pipelines = 0;
  for (int i = 1; i < argc; ++i) {
   @autoreleasepool {
    NSError* error = nil;
    NSString* source = [NSString stringWithContentsOfFile:[NSString stringWithUTF8String:argv[i]] encoding:NSUTF8StringEncoding error:&error];
    // Exactly the production embedded-source loader's compile options.
    id<MTLLibrary> library = [device newLibraryWithSource:source options:nil error:&error];
    if (!library) {
     fprintf(stderr, "FAILED %s: %s\n", argv[i], error.localizedDescription.UTF8String);
     ++failures;
     continue;
    }
    id<MTLFunction> function = [library newFunctionWithName:@"entry_xe"];
    if (!function) {
     fprintf(stderr, "FAILED %s: entry_xe missing\n", argv[i]);
     ++failures;
    } else if (function.functionType == MTLFunctionTypeKernel) {
     id<MTLComputePipelineState> pipeline = [device newComputePipelineStateWithFunction:function error:&error];
     if (!pipeline) {
      fprintf(stderr, "FAILED %s pipeline: %s\n", argv[i], error.localizedDescription.UTF8String);
      ++failures;
     } else { ++pipelines; }
     [pipeline release];
    }
    [function release];
    [library release];
   }
  }
  fprintf(stderr, "Validated %d embedded sources and %d compute pipelines, %d failures\n", argc-1, pipelines, failures);

  NSError* error = nil;
  NSString* smoke = @"#include <metal_stdlib>\nusing namespace metal;\nkernel void gpu_readback_probe(device uint* out [[buffer(0)]], uint i [[thread_position_in_grid]]) { out[i] = i * 3u + 7u; }";
  id<MTLLibrary> library = [device newLibraryWithSource:smoke options:nil error:&error];
  id<MTLFunction> function = [library newFunctionWithName:@"gpu_readback_probe"];
  id<MTLComputePipelineState> pipeline = [device newComputePipelineStateWithFunction:function error:&error];
  id<MTLCommandQueue> queue = [device newCommandQueue];
  id<MTLBuffer> buffer = [device newBufferWithLength:64*sizeof(uint32_t) options:MTLResourceStorageModeShared];
  if (!library || !function || !pipeline || !queue || !buffer) {
    fprintf(stderr, "FAILED GPU readback setup: %s\n", error.localizedDescription.UTF8String);
    ++failures;
  } else {
    memset(buffer.contents, 0, buffer.length);
    id<MTLCommandBuffer> command = [queue commandBuffer];
    id<MTLComputeCommandEncoder> encoder = [command computeCommandEncoder];
    [encoder setComputePipelineState:pipeline];
    [encoder setBuffer:buffer offset:0 atIndex:0];
    [encoder dispatchThreadgroups:MTLSizeMake(1,1,1) threadsPerThreadgroup:MTLSizeMake(64,1,1)];
    [encoder endEncoding];
    [command commit];
    [command waitUntilCompleted];
    bool passed = command.status == MTLCommandBufferStatusCompleted;
    for (uint32_t i = 0; i < 64; ++i) {
      passed &= ((uint32_t*)buffer.contents)[i] == i*3+7;
    }
    fprintf(stderr, "GPU dispatch + shared-buffer readback: %s (64 values)\n", passed ? "PASS" : "FAIL");
    if (!passed) ++failures;
  }
  [buffer release]; [queue release]; [pipeline release]; [function release]; [library release];
  [device release];
  return failures ? 1 : 0;
 }
}

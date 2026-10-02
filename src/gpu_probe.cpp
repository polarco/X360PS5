// SPDX-License-Identifier: GPL-3.0-or-later
#include "diagnostics.hpp"
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <cstring>
#include <vector>
#include "probe_shader.hpp"
#if X360_PS5
extern "C" PFN_vkVoidFunction vk_icdGetInstanceProcAddr(VkInstance,const char*);
#else
#include <dlfcn.h>
#endif
namespace x360 {
namespace {
bool gpu_blocked=false;
#define VK_FUNCTIONS(X) \
X(CreateInstance) X(DestroyInstance) X(EnumeratePhysicalDevices) \
X(GetPhysicalDeviceProperties) X(GetPhysicalDeviceFeatures) X(GetPhysicalDeviceMemoryProperties) \
X(GetPhysicalDeviceQueueFamilyProperties) X(CreateDevice) X(DestroyDevice) X(GetDeviceQueue) \
X(CreateBuffer) X(DestroyBuffer) X(GetBufferMemoryRequirements) X(AllocateMemory) X(FreeMemory) \
X(BindBufferMemory) X(MapMemory) X(UnmapMemory) X(InvalidateMappedMemoryRanges) \
X(CreateShaderModule) X(DestroyShaderModule) X(CreateDescriptorSetLayout) X(DestroyDescriptorSetLayout) \
X(CreatePipelineLayout) X(DestroyPipelineLayout) X(CreateComputePipelines) X(DestroyPipeline) \
X(CreateDescriptorPool) X(DestroyDescriptorPool) X(AllocateDescriptorSets) X(UpdateDescriptorSets) \
X(CreateCommandPool) X(DestroyCommandPool) X(AllocateCommandBuffers) X(BeginCommandBuffer) X(EndCommandBuffer) \
X(CmdBindPipeline) X(CmdBindDescriptorSets) X(CmdDispatch) X(CmdPipelineBarrier) \
X(CreateFence) X(DestroyFence) X(QueueSubmit) X(WaitForFences) \
X(CreateImage) X(DestroyImage) X(GetImageMemoryRequirements) X(BindImageMemory) X(CmdClearColorImage) X(CmdCopyImageToBuffer)
struct Gpu {
#define DECL(n) PFN_vk##n n=nullptr;
  VK_FUNCTIONS(DECL)
#undef DECL
  PFN_vkGetInstanceProcAddr gipa=nullptr;
  VkInstance instance{}; VkPhysicalDevice physical{}; VkDevice device{}; VkQueue queue{};
  VkBuffer buffer{}; VkDeviceMemory memory{}; VkImage image{}; VkDeviceMemory image_memory{};
  VkShaderModule shader{}; VkDescriptorSetLayout descriptor_layout{}; VkPipelineLayout layout{};
  VkPipeline pipeline{}; VkDescriptorPool descriptors{}; VkCommandPool commands{}; VkFence fence{};
  void* mapped=nullptr; bool timed_out=false; unsigned family=0;
  bool coherent=false; VkPhysicalDeviceMemoryProperties memories{};
#if !X360_PS5
  void* library=nullptr;
#endif
  ~Gpu() {
    // A timeout does not prove the queue is idle. Do not free resources still in use.
    if(timed_out) return;
    if(device) {
      if(mapped) UnmapMemory(device,memory);
      if(fence) DestroyFence(device,fence,nullptr);
      if(commands) DestroyCommandPool(device,commands,nullptr);
      if(descriptors) DestroyDescriptorPool(device,descriptors,nullptr);
      if(pipeline) DestroyPipeline(device,pipeline,nullptr);
      if(layout) DestroyPipelineLayout(device,layout,nullptr);
      if(descriptor_layout) DestroyDescriptorSetLayout(device,descriptor_layout,nullptr);
      if(shader) DestroyShaderModule(device,shader,nullptr);
      if(image) DestroyImage(device,image,nullptr);
      if(image_memory) FreeMemory(device,image_memory,nullptr);
      if(buffer) DestroyBuffer(device,buffer,nullptr);
      if(memory) FreeMemory(device,memory,nullptr);
      DestroyDevice(device,nullptr);
    }
    if(instance && DestroyInstance) DestroyInstance(instance,nullptr);
#if !X360_PS5
    if(library) dlclose(library);
#endif
  }
  int type(unsigned bits,VkMemoryPropertyFlags required) {
    for(unsigned i=0;i<memories.memoryTypeCount;++i)
      if((bits&(1u<<i)) && (memories.memoryTypes[i].propertyFlags&required)==required) return int(i);
    return -1;
  }
};
bool cycle(Report& report, unsigned iteration) {
  Gpu g;
#if X360_PS5
  g.gipa=vk_icdGetInstanceProcAddr;
#else
  g.library=dlopen("libvulkan.so.1",RTLD_NOW|RTLD_LOCAL);
  if(g.library) g.gipa=reinterpret_cast<PFN_vkGetInstanceProcAddr>(dlsym(g.library,"vkGetInstanceProcAddr"));
#endif
  auto fail=[&](const char* op,int result) { report.record("vulkan",Status::failed,std::string(op)+" result="+std::to_string(result)); return false; };
  if(!g.gipa) return fail("Vulkan loader unavailable",-1);
  g.CreateInstance=reinterpret_cast<PFN_vkCreateInstance>(g.gipa(nullptr,"vkCreateInstance"));
  if(!g.CreateInstance) return fail("vkCreateInstance unavailable",-1);
  VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO}; app.pApplicationName="X360PS5 diagnostics"; app.apiVersion=VK_API_VERSION_1_1;
  VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO}; ici.pApplicationInfo=&app;
#define CHECK(expr) do { VkResult rc=(expr); if(rc!=VK_SUCCESS) return fail(#expr,rc); } while(0)
  CHECK(g.CreateInstance(&ici,nullptr,&g.instance));
#define LOAD(n) g.n=reinterpret_cast<PFN_vk##n>(g.gipa(g.instance,"vk" #n)); if(!g.n) return fail("missing vk" #n,-1);
  VK_FUNCTIONS(LOAD)
#undef LOAD
  unsigned count=0; CHECK(g.EnumeratePhysicalDevices(g.instance,&count,nullptr));
  if(!count) return fail("no GPU",-1);
  std::vector<VkPhysicalDevice> devices(count); CHECK(g.EnumeratePhysicalDevices(g.instance,&count,devices.data())); g.physical=devices[0];
  VkPhysicalDeviceProperties properties{}; g.GetPhysicalDeviceProperties(g.physical,&properties);
  VkPhysicalDeviceFeatures features{}; g.GetPhysicalDeviceFeatures(g.physical,&features);
  if(!iteration) {
    report.note(std::string("GPU=")+properties.deviceName+" Vulkan="+std::to_string(properties.apiVersion)+" driver="+std::to_string(properties.driverVersion));
    report.note("features shaderInt64="+std::to_string(features.shaderInt64)+" geometryShader="+std::to_string(features.geometryShader)+" tessellationShader="+std::to_string(features.tessellationShader));
  }
  g.GetPhysicalDeviceMemoryProperties(g.physical,&g.memories);
  count=0; g.GetPhysicalDeviceQueueFamilyProperties(g.physical,&count,nullptr);
  std::vector<VkQueueFamilyProperties> families(count); g.GetPhysicalDeviceQueueFamilyProperties(g.physical,&count,families.data());
  bool found=false;
  for(unsigned i=0;i<count;++i) if(families[i].queueCount && (families[i].queueFlags&VK_QUEUE_COMPUTE_BIT)) { g.family=i; found=true; break; }
  if(!found) return fail("no compute queue",-1);
  float priority=1; VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO}; qci.queueFamilyIndex=g.family; qci.queueCount=1; qci.pQueuePriorities=&priority;
  VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO}; dci.queueCreateInfoCount=1; dci.pQueueCreateInfos=&qci;
  CHECK(g.CreateDevice(g.physical,&dci,nullptr,&g.device)); g.GetDeviceQueue(g.device,g.family,0,&g.queue);
  VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO}; bci.size=4096; bci.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  CHECK(g.CreateBuffer(g.device,&bci,nullptr,&g.buffer));
  VkMemoryRequirements req{}; g.GetBufferMemoryRequirements(g.device,g.buffer,&req);
  int type=g.type(req.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
  if(type<0) return fail("no host visible memory",-1);
  g.coherent=(g.memories.memoryTypes[type].propertyFlags&VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)!=0;
  VkMemoryAllocateInfo mai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO}; mai.allocationSize=req.size; mai.memoryTypeIndex=type;
  CHECK(g.AllocateMemory(g.device,&mai,nullptr,&g.memory)); CHECK(g.BindBufferMemory(g.device,g.buffer,g.memory,0));
  VkShaderModuleCreateInfo sm{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO}; sm.codeSize=sizeof(probe_shader); sm.pCode=probe_shader;
  CHECK(g.CreateShaderModule(g.device,&sm,nullptr,&g.shader));
  VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1,VK_SHADER_STAGE_COMPUTE_BIT,nullptr};
  VkDescriptorSetLayoutCreateInfo sl{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO}; sl.bindingCount=1; sl.pBindings=&binding;
  CHECK(g.CreateDescriptorSetLayout(g.device,&sl,nullptr,&g.descriptor_layout));
  VkPipelineLayoutCreateInfo pl{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO}; pl.setLayoutCount=1; pl.pSetLayouts=&g.descriptor_layout;
  CHECK(g.CreatePipelineLayout(g.device,&pl,nullptr,&g.layout));
  VkComputePipelineCreateInfo cp{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO}; cp.layout=g.layout;
  cp.stage={VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO}; cp.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT; cp.stage.module=g.shader; cp.stage.pName="main";
  CHECK(g.CreateComputePipelines(g.device,VK_NULL_HANDLE,1,&cp,nullptr,&g.pipeline));
  VkDescriptorPoolSize poolsize{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,1};
  VkDescriptorPoolCreateInfo dp{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO}; dp.maxSets=1; dp.poolSizeCount=1; dp.pPoolSizes=&poolsize;
  CHECK(g.CreateDescriptorPool(g.device,&dp,nullptr,&g.descriptors));
  VkDescriptorSetAllocateInfo da{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO}; da.descriptorPool=g.descriptors; da.descriptorSetCount=1; da.pSetLayouts=&g.descriptor_layout;
  VkDescriptorSet set{}; CHECK(g.AllocateDescriptorSets(g.device,&da,&set));
  VkDescriptorBufferInfo db{g.buffer,0,1024}; VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET}; write.dstSet=set; write.descriptorCount=1; write.descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; write.pBufferInfo=&db;
  g.UpdateDescriptorSets(g.device,1,&write,0,nullptr);
  VkImageCreateInfo im{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO}; im.imageType=VK_IMAGE_TYPE_2D; im.format=VK_FORMAT_R8G8B8A8_UNORM; im.extent={8,8,1}; im.mipLevels=1; im.arrayLayers=1; im.samples=VK_SAMPLE_COUNT_1_BIT; im.tiling=VK_IMAGE_TILING_OPTIMAL; im.usage=VK_IMAGE_USAGE_TRANSFER_DST_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  CHECK(g.CreateImage(g.device,&im,nullptr,&g.image)); g.GetImageMemoryRequirements(g.device,g.image,&req);
  type=g.type(req.memoryTypeBits,0); if(type<0) return fail("image memory type",-1);
  mai.allocationSize=req.size; mai.memoryTypeIndex=type; CHECK(g.AllocateMemory(g.device,&mai,nullptr,&g.image_memory)); CHECK(g.BindImageMemory(g.device,g.image,g.image_memory,0));
  VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO}; pci.queueFamilyIndex=g.family;
  CHECK(g.CreateCommandPool(g.device,&pci,nullptr,&g.commands));
  VkCommandBufferAllocateInfo ca{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO}; ca.commandPool=g.commands; ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY; ca.commandBufferCount=1;
  VkCommandBuffer command{}; CHECK(g.AllocateCommandBuffers(g.device,&ca,&command));
  VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO}; CHECK(g.BeginCommandBuffer(command,&begin));
  g.CmdBindPipeline(command,VK_PIPELINE_BIND_POINT_COMPUTE,g.pipeline); g.CmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_COMPUTE,g.layout,0,1,&set,0,nullptr); g.CmdDispatch(command,4,1,1);
  VkImageSubresourceRange range{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
  VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER}; barrier.oldLayout=VK_IMAGE_LAYOUT_UNDEFINED; barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; barrier.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED; barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED; barrier.image=g.image; barrier.subresourceRange=range; barrier.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
  g.CmdPipelineBarrier(command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
  VkClearColorValue red{{1,0,0,1}}; g.CmdClearColorImage(command,g.image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,&red,1,&range);
  barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; barrier.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT; barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
  g.CmdPipelineBarrier(command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&barrier);
  VkBufferImageCopy copy{}; copy.bufferOffset=1024; copy.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1}; copy.imageExtent={8,8,1};
  g.CmdCopyImageToBuffer(command,g.image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,g.buffer,1,&copy);
  VkMemoryBarrier hostbar{VK_STRUCTURE_TYPE_MEMORY_BARRIER}; hostbar.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_TRANSFER_WRITE_BIT; hostbar.dstAccessMask=VK_ACCESS_HOST_READ_BIT;
  g.CmdPipelineBarrier(command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&hostbar,0,nullptr,0,nullptr);
  CHECK(g.EndCommandBuffer(command)); VkFenceCreateInfo fc{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO}; CHECK(g.CreateFence(g.device,&fc,nullptr,&g.fence));
  VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO}; submit.commandBufferCount=1; submit.pCommandBuffers=&command; CHECK(g.QueueSubmit(g.queue,1,&submit,g.fence));
  VkResult waited=g.WaitForFences(g.device,1,&g.fence,VK_TRUE,5000000000ull);
  if(waited!=VK_SUCCESS) { g.timed_out=true; gpu_blocked=true; return fail("GPU fence; stop testing and close app",waited); }
  CHECK(g.MapMemory(g.device,g.memory,0,VK_WHOLE_SIZE,0,&g.mapped));
  if(!g.coherent) { VkMappedMemoryRange mr{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE}; mr.memory=g.memory; mr.size=VK_WHOLE_SIZE; CHECK(g.InvalidateMappedMemoryRanges(g.device,1,&mr)); }
  auto* values=static_cast<std::uint32_t*>(g.mapped); bool compute=true,image=true;
  for(unsigned i=0;i<256;++i) compute &= values[i]==((i*1664525u+1013904223u)^0xa5a55a5au);
  for(unsigned i=256;i<320;++i) image &= values[i]==0xff0000ffu;
  report.record("compute",compute?Status::passed:Status::failed,"256 uints checked against CPU oracle; cycle="+std::to_string(iteration));
  report.record("gpu_readback",image?Status::passed:Status::failed,"8x8 RGBA red clear/copy readback; offscreen only");
  report.record("vulkan",compute&&image?Status::passed:Status::failed,"device, queue, resources and shader dispatch");
  return compute&&image;
#undef CHECK
}
}
void run_gpu(Report& r) {
  if(gpu_blocked) { r.record("vulkan",Status::failed,"GPU testing disabled after fence failure; restart app"); return; }
  for(unsigned i=0;i<3;++i) {
    r.begin("vulkan"); r.begin("compute"); r.begin("gpu_readback");
    if(!cycle(r,i)) return;
  }
}
}

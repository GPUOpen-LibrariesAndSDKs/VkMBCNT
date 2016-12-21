Vulkan shader extensions
========================

Welcome to the Vulkan&trade; `mbcnt` sample, which shows how to use the `AMD_shader_ballot` extension and `mbcnt` to perform a fast reduction within a wavefront.

License
-------

MIT: see `LICENSE.txt` for details.

System requirements
-------------------

* AMD Radeon&trade; GCN-based GPU (HD 7000 series or newer)
  * Or other Vulkan&trade; compatible discrete GPU 
* 64-bit Windows&reg; 7 (SP1 with the [Platform Update](https://msdn.microsoft.com/en-us/library/windows/desktop/jj863687.aspx)), Windows&reg; 8.1, or Windows&reg; 10
* Visual Studio&reg; 2013 or Visual Studio&reg; 2015
* Graphics driver with Vulkan support
  * For example, AMD Radeon Software Crimson Edition 16.5.2 or later
* The [Vulkan SDK](https://vulkan.lunarg.com) must be installed

Building
--------

Visual Studio files can be found in the `vkmbcnt\build` directory.

If you need to regenerate the Visual Studio files, open a command prompt in the `vkmbcnt\premake` directory and run `..\..\premake\premake5.exe vs2015` (or `..\..\premake\premake5.exe vs2013` for Visual Studio 2013.)

Third-party software
------------------

* Premake is distributed under the terms of the BSD License. See `premake\LICENSE.txt`.

Attribution
-----------

* AMD, the AMD Arrow logo, Radeon, and combinations thereof are either registered trademarks or trademarks of Advanced Micro Devices, Inc. in the United States and/or other countries.
* Microsoft, Visual Studio, and Windows are either registered trademarks or trademarks of Microsoft Corporation in the United States and/or other countries.
* Vulkan and the Vulkan logo are trademarks of the Khronos Group, Inc.

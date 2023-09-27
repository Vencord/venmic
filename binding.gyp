{
  "targets": [
    {
      "target_name": "vencordvirtmic",
      "sources": [ "src/main.cpp", "src/virtmic.cpp" ],
      "include_dirs": [ "src", "submodules/rohrkabel/include", "/usr/lib64" ],
      "libraries": ["<(module_root_dir)/submodules/rohrkabel/build/librohrkabel.a", "-lpipewire-0.3"],
    }
  ]
} 
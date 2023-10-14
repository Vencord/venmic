<div align="center">

<img src="https://avatars.githubusercontent.com/u/113042587" width="150">

<br/>

venmic - screenshare support for pipewire

</div>

## 📖 Usage

_venmic_ can be used as node-module or as a local rest-server.

The node-module is intended for internal usage by [Vesktop](https://github.com/Vencord/Vesktop).

The Rest-Server exposes three simple endpoints
* (GET) `/list`
  > List all available applications to share

* (POST) `/link`
  > Expects a JSON-Body containing the target application, i.e. `{"key": "node.name", "value": "Firefox", "mode": "include"}`  
  > Valid values for `mode` are:
  > * `include`  
  >    The specified application will be shared
  > * `exclude`  
  >    All _but_ the specified application will be shared

* (GET) `/unlink`
  > Unlinks the currently linked application

## 🏗️ Compiling

* Rest-Server
    ```bash
    git clone https://github.com/Vencord/linux-virtmic && cd linux-virtmic
    cmake -B build && cmake --build build
    ```

* Node-Addon
    ```bash
    git clone https://github.com/Vencord/linux-virtmic && cd linux-virtmic
    pnpm install
    ```

## 🤝 Acknowledgements

This project heavily relies on the following projects:

* [Soundux/rohrkabel](https://github.com/Soundux/rohrkabel/)
* [cmake-js](https://github.com/cmake-js/cmake-js)

Kudos to all the developers involved, keep up the great work!

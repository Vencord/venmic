<div align="center">

<img src="https://avatars.githubusercontent.com/u/113042587" width="150">

<br/>

venmic - screenshare support for pipewire

</div>

> [!WARNING]  
> This project is not intended for standalone usage. You need a modified discord client that makes use of this.

## 📖 Usage

_venmic_ can be used as node-module or as a local rest-server.

The node-module is intended for internal usage by [Vesktop](https://github.com/Vencord/Vesktop).

The Rest-Server exposes three simple endpoints
* (POST) `/list`
  > List all available applications to share.  
  > You can optionally define a JSON-Body containing all the props the listed nodes should have (i.e. `["node.name"]`).

* (POST) `/link`  
  <blockquote>
  Expects a JSON-Body in the following form:
  <pre lang="json">
  {
    "include": 
    [
      { "key": "node.name", "value": "Firefox" }
    ],
    "exclude":
    [
      { "key": "node.name", "value": "Chrome" }
    ]
  }
  </pre>

  Depending on wether or not `include` or `exclude` are defined the behavior will change:

  * only `include`
    * Links nodes that match given props
  * only `exclude`
    * Links nodes that do not match given props
  * both `include` and `exclude`
    * Links all applications that match props in `include` and not those given in `exclude`
  </blockquote>

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

## 🐛 Debugging

When reporting an issue please make sure to set the environment variable `VENMIC_ENABLE_LOG`.

If said variable is set venmic will output a lot of useful information to stdout and a log-file which can be found in `~/.local/state/venmic/venmic.log`.

It is highly recommended to include this log file in your issue report otherwise we may not be able to help you!

## 🤝 Acknowledgements

This project heavily relies on the following projects:

* [Curve/rohrkabel](https://github.com/Curve/rohrkabel/)
* [cmake-js](https://github.com/cmake-js/cmake-js)

Kudos to all the developers involved, keep up the great work!

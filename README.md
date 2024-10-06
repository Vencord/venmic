<div align="center">

<img src="https://avatars.githubusercontent.com/u/113042587" width="150">

<br/>

venmic - screenshare support for pipewire

</div>

> [!WARNING]  
> This project is not intended for standalone usage. You need a modified discord client that makes use of this.

## 📖 Usage

_venmic_ can be used as node-module or as a local rest-server.

The node-module is mainly intended for internal usage by [Vesktop](https://github.com/Vencord/Vesktop).
For a usage example, see the following Vesktop source files:
- [src/main/venmic.ts](https://github.com/Vencord/Vesktop/blob/main/src/main/venmic.ts)
- [src/renderer/patches/screenShareFixes.ts](https://github.com/Vencord/Vesktop/blob/main/src/renderer/patches/screenShareFixes.ts)
- src/renderer/components/ScreenSharePicker.tsx: [1](https://github.com/Vencord/Vesktop/blob/4abae9c7082081dcae667916d9608e23adf688a9/src/renderer/components/ScreenSharePicker.tsx#L109-L115), [2](https://github.com/Vencord/Vesktop/blob/4abae9c7082081dcae667916d9608e23adf688a9/src/renderer/components/ScreenSharePicker.tsx#L253-L256), [3](https://github.com/Vencord/Vesktop/blob/4abae9c7082081dcae667916d9608e23adf688a9/src/renderer/components/ScreenSharePicker.tsx#L94)

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
      { "node.name": "Firefox" }
    ],
    "exclude":
    [
      { "node.name": "Chrome" }
    ]
    "ignore_devices": true,
    "workaround": [{ "node.name": "Chrome" }]
  }
  </pre>

  Depending on wether or not `include` or `exclude` are defined the behavior will change:

  * only `include`
    * Links nodes that match given props
  * only `exclude`
    * Links nodes that do not match given props
  * both `include` and `exclude`
    * Links all applications that match props in `include` and not those given in `exclude`

  The setting `ignore_devices` is optional and will default to `true`.  
  When enabled it will prevent hardware-devices like speakers and microphones from being linked to the virtual microphone.

  The setting `only_speakers` is optional and will default to `true`.  
  When enabled it will prevent linking against nodes that don't play to a speaker.

  The setting `only_default_speakers` is optional and will default to `true`.  
  When enabled it will prevent linking against nodes that don't play to the default speaker.

  The setting `workaround` is also optional and will default to an empty array.  
  When set, venmic will redirect the first node that matches all of the specified properties to itself.
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

* [Curve/rohrkabel](https://github.com/Curve/rohrkabel/)
* [cmake-js](https://github.com/cmake-js/cmake-js)
* [@wwmm](https://github.com/wwmm) for improving compatibility with [EasyEffects](https://github.com/wwmm/easyeffects)

Kudos to all the developers involved, keep up the great work!

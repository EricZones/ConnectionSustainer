# ConnectionSustainer

ConnectionSustainer is a Teamspeak client plugin that keeps you stay connected on every server you are connected to. It protects you against kicks and one second bans.

## Features
- Reconnecting after kick from server
- Reconnecting after ban from server
- Automated switch to last channel

## Installation & Execution
### Requirements
- Teamspeak 3 Client 3.6.2
- CMake 3.30

### Build process
```bash
  cmake .
  
  cmake --build .
  ```

### Execution
1. Move 'ConnectionSustainer.dll' into your Teamspeak plugins directory
2. Create a folder named 'ConnectionSustainer' in the plugins directory
3. Move all images from 'src/icons/' into the created 'ConnectionSustainer' directory
4. Start your Teamspeak client and enable the plugin with the 'Plugins' button

## Controls
| Button         | Description          |
|----------------|----------------------|
| Enable Plugin  | Enable reconnecting  |
| Disable Plugin | Disable reconnecting |

- Buttons can be accessed by clicking on the 'Plugins' button on the top bar

## Contributors
- **EricZones** - Developer
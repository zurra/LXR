# LXR

<p align="center">
  <img src="https://docs.clusterfact.games/lxr/logo-anim.gif" alt="LXR Logo" width="300">
</p>

## The Ultimate Lighting Detection and Perception Plugin for Unreal Engine

LXR is a lighting detection and perception plugin for Unreal Engine. Its primary purpose is to measure how much light an actor is receiving, but that is only the starting point.

LXR provides a broader set of light-awareness tools for stealth systems, AI perception, environmental interaction, and gameplay mechanics built around direct light, indirect light, silhouettes, light volumes, and remembered light states.

The plugin is designed to be fast, practical, and easy to integrate into real projects.

## Core Use Cases

LXR can be used for:

* Stealth visibility systems
* AI light awareness
* Flashlight and searchlight detection
* Silhouette detection against bright backgrounds
* Light-based environmental puzzles
* Detection of direct and indirect illumination
* EQS-based light queries
* Runtime gameplay reactions to changing light states

## Direct Light Detection

Direct light detection measures how much light an actor receives from explicit light sources.

This requires:

* An LXR Detection Component on the actor whose illumination should be evaluated
* LXR Source Components on actors that contain Unreal Engine Light Components

LXR Source Components can be added to actors containing light sources, after which LXR can evaluate how much direct light reaches detected actors.

The system supports Unreal Engine light types and provides information about both illumination intensity and received light color.

## Indirect Light Detection

LXR also supports indirect light detection.

Indirect light detection requires only an LXR Flux Component on the actor whose illumination should be evaluated. No additional source setup is needed.

This allows LXR to evaluate indirect lighting intensity and color, including lighting influenced by Unreal Engine's modern global illumination systems.

The indirect lighting system of LXR is open source and available on GitHub.

## Key Components

### Detection Component

The Detection Component evaluates how visible an actor is to relevant light sources.

It can determine:

* How much light the actor is receiving
* Which light sources are affecting the actor
* The approximate color of received light
* Whether the actor is sufficiently illuminated for gameplay purposes

This is the main component used for traditional light-based stealth detection.

### Source Component

The Source Component marks an actor as an LXR-recognized light source.

Add an LXR Source Component to an actor that contains a Light Component, and LXR can use it for direct light detection.

### Flux Component

The Flux Component is used for indirect light detection.

It allows an actor to evaluate received indirect light without requiring explicit LXR Source Components on nearby lights.

### Sense Component

The Sense Component allows AI actors to perceive light volumes in their environment.

This enables AI to react to light itself, not only to illuminated actors.

For example, an AI can detect a player's flashlight beam around an L-shaped corridor before the player becomes directly visible. The AI is not detecting the player; it is detecting the visible light volume.

This is useful for stealth, horror, tactical AI, and any gameplay where light sources should create suspicion or awareness.

### Silhouette Component

The Silhouette Component detects characters or actors silhouetted against bright backgrounds.

This solves a common problem in stealth games: standing in darkness should not always mean being invisible. If a character stands in a dark corridor with a bright opening behind them, their silhouette can still be visible.

The Silhouette Component enables AI to respond to that scenario.

### Memory Component

The Memory Component allows AI actors to remember environmental light states.

AI can remember whether lights were on or off in specific locations, even after leaving and returning to the area. This allows AI to react when the player changes the environment, such as turning lights off, turning lights on, or altering the lighting state of a room.

Memory can work together with:

* Detection Component, when the actor is inside a light volume
* Sense Component, when the actor can perceive the light volume from a distance

## Performance

LXR is built with performance in mind.

### Multi-threaded Optimization

LXR uses multithreaded processing where appropriate to keep lighting checks fast and scalable.

### Smart Update Intervals

LXR can dynamically adjust check intervals based on runtime conditions such as distance and relevance.

This allows nearby or important actors to be checked more frequently, while distant or less relevant actors can be updated less often.

### Octree Support

LXR includes Octree support for fast spatial queries.

This allows the plugin to quickly find nearby light sources and relevant LXR actors, making it more suitable for larger environments and more demanding gameplay scenarios.

## Physically Based Illuminance

LXR can calculate illuminance using physically based lighting principles.

This allows gameplay systems to work with meaningful light intensity values rather than relying only on rough approximations.

## EQS Support

LXR includes full Environmental Query System support.

This allows AI systems to query lighting information at locations in the world, such as:

* How illuminated a location is
* What color of light affects a location
* Whether a location is suitable for hiding
* Whether a location is exposed to light

## Async World Queries

LXR also includes QueryLXRAsyncTask, which can be used to query the amount of light at any world location asynchronously.

This is useful for systems that need lighting information without tying the query directly to a specific actor component.

## Debugging

LXR includes tools to make lighting detection easier to inspect and troubleshoot.

### Clear Log Messages

LXR provides informative log output to help understand what the plugin is doing and diagnose setup or runtime issues.

### Visual Debugging

LXR includes visual debugging tools for inspecting light sources, detection behavior, and interactions between actors and lighting.

### Console Commands

LXR includes built-in debugging commands for troubleshooting and experimentation.

## Documentation

The documentation aims to cover common setup patterns, component usage, debugging workflows, and typical gameplay use cases.

LXR is designed to be practical for production use, not just a technical demo.

## Support

LXR-related help is available through Discord.

## License

LXR is source-available under the LXR Community License.

### TL;DR

You may:

* Use LXR in personal projects
* Use LXR in commercial games
* Modify the source code
* Create internal company forks
* Learn from and contribute to the project
* Ship games that include LXR

You may not:

* Sell LXR itself
* Upload LXR or modified versions of LXR to Fab, Unreal Marketplace, GitHub releases, or other distribution channels
* Redistribute LXR binaries publicly
* Repackage LXR as your own plugin or competing product

If you are making a game with LXR, you are good.

If you are trying to sell LXR, do not.

See the LICENSE file for the full license terms. The TL;DR is provided for convenience only; the LICENSE file is the legally binding document.

# Video output modes

Open **Audio & Video → Video output**. This setting was called **Sync filter** in earlier builds. Its first three choices keep their previous settings when you load an existing configuration.

| Mode | What it does |
|---|---|
| **Filtered** | Standard output. Stabilizes timing for the MiSTer video chain and compensates pixel alignment. This was called **Full**. |
| **Raw pixels** | Keeps filtered timing and shows native pixel ordering and blanking. Useful for inspecting the core's raw pixel behavior. |
| **Raw CRT** | Uses the core's raw display timing and bypasses its crop, scandoubler and scanline processing. Use a display that accepts the resulting timing. |
| **CRT response** | Experimental. Models the horizontal movement caused by unusual sync pulses. Uses native pixel cadence and sends the shifted picture through the normal output pipeline. |

**CRT response** is available on builds that enable the experimental receiver. It forces native pixel cadence and bypasses adaptive pixel clock and HQ2x. Gamma, scanline/scandoubler settings, crop and HDMI scaling still apply where otherwise supported. A horizontal shift is expressed in native pixels, then scaled and clipped with the picture downstream.

The core normally switches modes during blanking. If a program produces timing that prevents the switch, reset to apply the selected mode.

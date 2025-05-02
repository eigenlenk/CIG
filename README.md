![logo_120](https://github.com/user-attachments/assets/44cbc982-b224-4c94-9afc-a704a7cdc77c)

**CIG** (**C** **I**mmediate **G**UI) is a simple layout and GUI framework written in C.

# Motivation
In many of the [game projects](https://eigen.itch.io) I do, UI tends to be prominent and take up a lot development time. In the past I've always rolled a retained-mode GUI which turns into a spaghetti code of allocating widgets, keeping references around and making sure to clear everyting once the element is closed or removed. It's also annoying to bind state and observe changes, often duplicating the state inside the widget itself. Once I got around to making [Cats On Broombas](https://eigen.itch.io/cats-on-broombas), and even though the UI isn't nearly as key there, I thought I'd try something different, namely an [immediate-mode GUI](https://caseymuratori.com/blog_0001). So then, **CIG** is an evolution of the ideas I experimented with there.

# Concept
In the process of making this a standalone library the architecture changed a little, but the main concept is laying out your UI using frames (mostly auto-sized) and observing interactions with that frame (mouse for now, but why not touch as well). Frames can be nested, clipped and scrolled. You push a frame, draw something in there and then pop. Or push a frame to define an area, and then push two more frames that get laid out equally horizontally, vertically or in any other way using a custom layout function. Frame defines a region for a widget, but isn't itself anything. A basic example looks like this:

```c
/* Begin laying out a UI */
cig_begin_layout(&context, <render target ref>, cig_rect_make(0, 0, 1024, 768), delta_time);

/* Pass input state */
cig_set_input_state(mouse_coords, mouse_buttons);

/* A 100x100 frame at the top-right corner. A minimap perhaps? */
if (cig_push_frame(cig_rect_make(CIG_W - 100, 0, 100, 100))) {
  /* Call game code to render the map */
  cig_pop_frame(); /* We are now back at the root frame */
}

 /* A 800x600 frame centered on screen. A modal window? */
if (cig_push_frame(RECT_CENTERED(800, 600))) {
  /* Inside the "modal window", we push a new frame with a custom layout function.
     This one helps us lay things out in a vertical stack. Stacks can also be
     horizontal, or have both axis enabled, in which case they become grids */
  cig_push_layout_function(&cig_default_layout_builder, RECT_AUTO, cig_insets_zero(), (cig_layout_params_t) {
    .axis = CIG_LAYOUT_AXIS_VERTICAL,
    .spacing = 10,
    .height = 100
  });
  
  /* Inserts 4 rows, each 100 units high and with 10 units between them */
  for (int i = 0; i < 4; ++i) {
    if (cig_push_frame(RECT_AUTO)) {
      /* Why is this an if-statement? In short, TRUE means the child frame is at least
         partially visible within the parent, FALSE means it's out of bounds. Relevant
         when we'd up the row count to, let's say 10, in which case the frames after the
         sixth one would not be visible, because row 6 is the last one partially in view */

      /* We want to be able to interact with this element */
      cig_enable_interaction();
  
      if (cig_clicked(CIG_INPUT_MOUSE_BUTTON_LEFT, CIG_CLICK_DEFAULT_OPTIONS)) {
        /* This row was clicked! */
        do_something();
      }
  
      cig_pop_frame();
    }
  }
  
  cig_pop_frame(); /* Pops the vertical stack */
  cig_pop_frame(); /* Pops the "modal window" */
}

cig_end_layout();
```

The secondary goal with this library is to keep the core layout separate from visuals or rendering backends. You just pass in a reference to your render target/texture/screen/buffer/whatever as an opaque pointer, and you get it back when it comes to rendering. The frame element is void of any graphical info - no background color, border or corner radiuses to deal with. The reason to separate this is that widgets come in so many different shapes and forms that it doesn't make sense to try codify that into fundamental types. A solid background color fits some GUI styles, but what if you want a gradient, or textured background? Components like windows can look so many different ways that it's best to leave that to the application/implementation layer. Of course the library can provide hooks and building blocks to make life easier.

# Demo
The demo serves as a work-in-progress testbed for the library. I like retro computing, so **obviously** I chose Windows 95 to ~~re~~demake. Rendering is done using Raylib as that was easy to get up and running with. But as mentioned earlier, nothing about CIG is tied to a particular renderer. Of course if you opt to use this library, you can integrate it more tightly with your graphics-ware and don't have to have another layer of abstraction like this.

![image](https://github.com/user-attachments/assets/5f05c3f4-b88b-417d-8189-6d771cbd6ed5)

# Getting started
The library uses [nob.h](https://github.com/tsoding/nob.h) to bootstrap the build command to build the test and demo target.

1. Use `gcc -o build build.c -std=gnu99` to create the builder (or `CC`, depending on your compiler situation)
2. Then run `build test` or `build demo`

# Structure
The project has 3 main modules for now.

### 1. Core
This is the main chunk of the whole thing. Has all the actual layout logic.

### 2. Text
Adds basic text element in form of a Label and deals with measuring and laying out text, as well as applying style through HTML-like tags.

### 3. Graphics
Adds an image element as well as line and rectangle drawing (pipings calls to the backend essentially).

# TODO
* Validate and refine the API by working on the demo
* Improve image element and pre-calculate more of the aspect ratio data before making a backend call
* Explore a concept of "anchors" to simplify layout creation
* Improve text support (for large pages of text with scrolling)
* Consider bundling commonly used widgets or at least establish best practices
* Improve and add missing tests (+there may be duplications)
* More documentation, examples and tutorials
  
## Acknowledgements
* Shoutout to Dear ImGui for introducing the concept of immediate-mode GUIs to wider audiences (including me) all these years ago
* More recently I came across [Clay](https://github.com/nicbarker/clay) which reinforced some of the ideas and encouraged me to continue

# What now?
This is pretty much my first open-source library, so if you find this interesting or it gives you an idea how to make that thing of your own, that's awesome. If you want to try and use it, that's even better! Willing to contribute a fix or a new feature? OMG! We can chat on [Discord](https://discord.gg/X379hyV37f) 👋

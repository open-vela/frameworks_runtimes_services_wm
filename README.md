# Window Manager

[ English | [简体中文](./README_zh-cn.md) ]

## Overview

The Window Manager Service (WMS) is a core service of the openvela operating system. It is primarily responsible for system input dispatching, output composition, and display management, ensuring smooth and stable user interface interactions.

### Core Functions

The Window Manager Service provides the following core capabilities:

- **Window Attribute & Style Management**: Supports dynamic adjustment of window position, size, opacity (Alpha), and hierarchy (Z-Order).
- **Lifecycle Management**: Provides control interfaces for the full lifecycle including window creation, display, hiding, and destruction.
- **Event Dispatching**: Manages the listening and dispatching of input events.
- **Transition Animations**: Supports transition animation management during window switching.

## Architecture

Window management in openvela adopts a Client-Server separation architecture, as shown in Figure 1.

**Figure 1** Window Manager Service Architecture Diagram

![Window Manager Service Architecture](./docs/Window_Manager_Architecture.png)

### Component Responsibilities

- **Window Manager Client (App Side)**

    - Runs in the **application User Space**.
    - Responsible for internal window management and graphics rendering within the application.
    - Submits rendered buffers to the server via IPC mechanisms.

- **Window Manager Server (Service Side)**

    - Runs as a core system service in **Kernel Space or as a system service process**.
    - Responsible for global window scheduling, hierarchy management, and the final composition of multi-window screens.

## Directory Structure

```text
├── app         # Application interfaces and examples
├── common      # Common data structures and utility libraries
├── config      # Configuration files
├── include     # Public header files
├── Kconfig     # Build configuration description file
├── server      # Core implementation of the Window Manager Service
└── test        # Test cases
```

## Constraints and Dependencies

Please adhere to the following constraints before developing or integrating the Window Manager Service:

- **Build Configuration**: Compilation options for the Window Manager Service must be configured via the `Kconfig` file.
- **Language Standard**: C++11 or higher.
- **System Dependency**: Must depend on the **Vela Core** service to run.

## Development Guide

This section describes how to call Window Manager interfaces in native applications.

### 1. Get Window Manager Service Instance

Obtain the client proxy object of the Window Manager Service by its service name.

```c++
WindowManager windowManager = (WindowManager) getService(WindowManager::name());
```

### 2. Create a Window

Configure the `LayoutParams` attributes and add a new window.

```c++
// Initialize window layout parameters
WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
layoutParams.type = WindowManager.LayoutParams.TYPE_APPLICATION;   // Set window type to application window
layoutParams.format = PixelFormat.FORMAT_RGB_888;                  // Set pixel format
layoutParams.width = WindowManager.LayoutParams.MATCH_PARENT;      // Width matches parent container
layoutParams.height = WindowManager.LayoutParams.MATCH_PARENT;     // Height matches parent container
layoutParams.x = 0;                                                // Initial X coordinate
layoutParams.y = 0;                                                // Initial Y coordinate
layoutParams.windowTransitionState = WindowManager.LayoutParams.WINDOW_TRANSITION_ENABLE; // Enable transition animation

// Create and add the window
BaseWindow window = new BaseWindow(context, this);
windowManager.addWindow(window, layoutParams, visibility);
```

The code above creates an application window and adds it to the Window Manager Service's window list for management.

### 3. Modify Window Attributes

Window layout parameters (such as position) can be dynamically updated while the window is running.

```c++
// Get current window layout parameters
WindowManager.LayoutParams layoutParams = getWindow().getLayoutParams();

// Modify coordinates
layoutParams.x = 200;
layoutParams.y = 200;

// Apply updated parameters
getWindow().setLayoutParams(layoutParams);
```

The code above modifies the current Activity's window position to (200, 200).

### 4. Destroy a Window

When a window is no longer needed, it should be explicitly removed to release resources.

```c++
windowManager.removeWindow(window);
```

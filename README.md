# Window Manager

[English|[简体中文](./README_zh-cn.md)]

## Introduction
The Window Management service is one of the most important services in the Vela operating system. It is responsible for input management, output management, and display management. 

The Window Management service is composed of two parts: the server-side and the application-side. The server-side is responsible for window management, scheduling, and composition between applications, while the application-side is responsible for window management and rendering within applications, and sends the rendered images to the server-side, while also receiving input events from the server-side. 

The Window Management service runs as a core capability in the kernel system service process, while the application-side window management runs in the application user space.

## Features
- Window attribute and style management: including adjusting window position, size, transparency, and other properties.
- Window lifecycle management: including window creation, display, hide and delete.
- Event listening management.
- Window transition animation management.

## Usage of Native Application-side Window Management

### Obtain the Window Management Service

To obtain an instance of the Window Management Service, you can use the following code:

```c++
WindowManager windowManager = (WindowManager) getService(WindowManager::name());
```

### Create a Window

Use WindowManager.LayoutParams to create a window, as shown in the sample code below:

```c++
WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
layoutParams.type = WindowManager.LayoutParams.TYPE_APPLICATION;
layoutParams.format = PixelFormat.FORMAT_RGB_888;
layoutParams.width = WindowManager.LayoutParams.MATCH_PARENT;
layoutParams.height = WindowManager.LayoutParams.MATCH_PARENT;
layoutParams.x = 0;
layoutParams.y = 0;
layoutParams.windowTransitionState = WindowManager.LayoutParams.WINDOW_TRANSITION_ENABLE;

BaseWindow window = new BaseWindow(context, this);
windowManager.addWindow(window, layoutParams, visibility);
```

The above code creates an application window and adds it to the window list managed by the Window Management Service.

### Modify Window Properties

To modify window properties, you can use the following code:

```c++
WindowManager.LayoutParams layoutParams = getWindow().getLayoutParams();
layoutParams.x = 200;
layoutParams.y = 200;
getWindow().setLayoutParams(layoutParams);
```

The above code modifies the position of the current activity's window to (200, 200).

### Delete a Window

To delete a window, you can use the following code:

```c++
windowManager.removeWindow(window);
```
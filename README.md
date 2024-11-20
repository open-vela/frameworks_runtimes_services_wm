# Window Manager

[English|[简体中文](./README_zh-cn.md)]

## Introduction
Window management is one of the services in the openvela system, which is mainly responsible for input management, output management and display management.

Window management consists of two parts: the server and the application:

- The server is responsible for implementing window management, scheduling and synthesis between applications.

- The application is responsible for window management and rendering within the application, and passes the rendered image to the server, while also receiving input events from the server.

As the core capability of the system, the window management service will run in the kernel system service process, and the application-side window management will run in the application user space.

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
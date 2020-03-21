English|[中文](Readme.md)

# Head Pose Estimation<a name="EN-US_TOPIC_0228461773"></a>

This application can run on the Atlas 200 DK to collect camera data in real time and predict head pose information in the video.

The current application adapts to  [DDK&RunTime](https://ascend.huawei.com/resources)  of 1.31.0.0 and later versions.

## Prerequisites<a name="section137245294533"></a>

Before deploying this sample, ensure that:

-   Mind Studio  has been installed.
-   The Atlas 200 DK developer board has been connected to  Mind Studio, the cross compiler has been installed, the SD card has been prepared, and basic information has been configured.

## Deployment<a name="section412811285117"></a>

You can use either of the following methods:

1.  Quick deployment: visit  [https://gitee.com/Atlas200DK/faster-deploy](https://gitee.com/Atlas200DK/faster-deploy).

    >![](public_sys-resources/icon-note.gif) **NOTE:**   
    >-   The quick deployment script can be used to deploy multiple samples rapidly. Select  **headposeestimation**.  
    >-   The quick deployment script automatically completes code download, model conversion, and environment variable configuration. To learn about the detailed deployment process, select the common deployment mode. Go to  **[2. Common deployment](#li3208251440)**.  

2.  <a name="li3208251440"></a>Common deployment: visit  [https://gitee.com/Atlas200DK/sample-README/tree/master/sample-headposeestimation](https://gitee.com/Atlas200DK/sample-README/tree/master/sample-headposeestimation).

    >![](public_sys-resources/icon-note.gif) **NOTE:**   
    >-   In this deployment mode, you need to manually download code, convert models, and configure environment variables. After that, you will have a better understanding of the process.  


## Build<a name="section7994174585917"></a>

1.  Open the project.

    Go to the directory that stores the decompressed installation package as the Mind Studio installation user in CLI mode, for example,  **$HOME/MindStudio-ubuntu/bin**. Run the following command to start Mind Studio:

    **./MindStudio.sh**

    Open the  **sample-headposeestimation**  project.

2.  Configure project information in the  **src/param\_configure.conf**  file.

    For details, see  [Figure 1](#en-us_topic_0219099583_fig0391184062214).

    **Figure  1**  Configuration file<a name="en-us_topic_0219099583_fig0391184062214"></a>  
    

    ![](figures/en-us_image_0219102037.gif)

    Content of the configuration file:

    ```
    remote_host=
    data_source=
    presenter_view_app_name=
    ```

    -   **remote\_host**: IP address of the Atlas 200 DK developer board
    -   **data\_source**: camera channel. The value can be  **Channel-1**  or  **Channel-2**. For details, see "Viewing the Channel to Which a Camera Belongs" in  [Atlas 200 DK User Guide](https://ascend.huawei.com/documentation).
    -   **presenter\_view\_app\_name**: value of  **View Name**  on the  **Presenter Server**  page, which must be unique. The value consists of at least one character and supports only uppercase letters, lowercase letters, digits, and underscores \(\_\).

    Configuration example:

    ```
    remote_host=192.168.1.2
    data_source=Channel-1
    presenter_view_app_name=video
    ```

    >![](public_sys-resources/icon-note.gif) **NOTE:**   
    >-   All the three parameters must be set. Otherwise, the build fails.  
    >-   Do not use double quotation marks \(""\) during parameter settings.  

3.  Run the  **deploy.sh**  script to adjust configuration parameters and download and compile the third-party library. Open the  **Terminal**  window of Mind Studio. By default, the home directory of the code is used. Run the  **deploy.sh**  script in the background to deploy the environment, as shown in  [Figure 2](#en-us_topic_0219099583_fig4285173816445).

    **Figure  2**  Running the deploy.sh script<a name="en-us_topic_0219099583_fig4285173816445"></a>  
    ![](figures/running-the-deploy-sh-script-34.png "running-the-deploy-sh-script-34")

    >![](public_sys-resources/icon-note.gif) **NOTE:**   
    >-   During the first deployment, if no third-party library is used, the system automatically downloads and builds the third-party library, which may take a long time. The third-party library can be directly used for the subsequent build.  
    >-   During deployment, select the IP address of the host that communicates with the developer board. Generally, the IP address is the IP address configured for the virtual NIC. If the IP address is in the same network segment as the IP address of the developer board, it is automatically selected for deployment. If they are not in the same network segment, you need to manually type the IP address of the host that communicates with the Atlas DK to complete the deployment.  

4.  Start building. Open Mind Studio and choose  **Build \> Build \> Build-Configuration**  from the main menu. The  **build**  and  **run**  folders are generated in the directory, as shown in  [Figure 3](#en-us_topic_0219099583_fig1625447397).

    **Figure  3**  Build and file generation<a name="en-us_topic_0219099583_fig1625447397"></a>  
    

    ![](figures/en-us_image_0219102172.gif)

    >![](public_sys-resources/icon-notice.gif) **NOTICE:**   
    >When you build a project for the first time,  **Build \> Build**  is unavailable. You need to choose  **Build \> Edit Build Configuration**  to set parameters before the build.  

5.  Start Presenter Server.<a name="en-us_topic_0219099583_fig93931954162222"></a> 

    Open the  **Terminal**  tab page on Mind Studio. By default, under the code storage path, run the following command to start the Presenter Server program of the head pose estimation application on the server, as shown in  [Figure 4](#en-us_topic_0219099583_fig423515251067).

    **bash run\_present\_server.sh**

    **Figure  4**  Starting Presenter Server<a name="en-us_topic_0219099583_fig423515251067"></a>  
    

    ![](figures/en-us_image_0219102188.jpg)

    When the message  **Please choose one to show the presenter in browser\(default: 127.0.0.1\):**  is displayed, type the IP address \(usually IP address for accessing Mind Studio\) used for accessing the Presenter Server service in the browser.

    Select the IP address used by the browser to access the Presenter Server service in  **Current environment valid ip list**, as shown in  [Figure 5](#en-us_topic_0219099583_fig999812514814).

    **Figure  5**  Project deployment<a name="en-us_topic_0219099583_fig999812514814"></a>  
    

    ![](figures/en-us_image_0219102259.jpg)

    [Figure 6](#en-us_topic_0219099583_fig69531305324)  shows that the Presenter Server service has been started successfully.

    **Figure  6**  Starting the Presenter Server process<a name="en-us_topic_0219099583_fig69531305324"></a>  
    

    ![](figures/en-us_image_0219102274.jpg)

    Use the URL shown in the preceding figure to log in to Presenter Server \(only Google Chrome is supported\). The IP address is that typed in  [Figure 5](#en-us_topic_0219099583_fig999812514814)  and the default port number is  **7007**. The following figure indicates that Presenter Server has been started successfully.

    **Figure  7**  Home page<a name="en-us_topic_0219099583_fig64391558352"></a>  
    ![](figures/home-page-35.png "home-page-35")

    The following figure shows the IP address used by Presenter Server and  Mind Studio  to communicate with the Atlas 200 DK.

    **Figure  8**  IP address example<a name="en-us_topic_0219099583_fig1881532172010"></a>  
    ![](figures/ip-address-example-36.png "ip-address-example-36")

    In the figure:

    -   The IP address of the Atlas 200 DK developer board is  **192.168.1.2**  \(connected in USB mode\).
    -   The IP address used by Presenter Server to communicate with the Atlas 200 DK is in the same network segment as the IP address of the Atlas 200 DK on the UI Host server. For example:  **192.168.1.223**.
    -   The following describes how to access the IP address \(such as  **10.10.0.1**\) of Presenter Server using a browser. Because Presenter Server and  Mind Studio  are deployed on the same server, you can access  Mind Studio  through the browser using the same IP address. 


## Run<a name="section551710297235"></a>

1.  Run the head pose estimation program.

    On the toolbar of Mind Studio, click  **Run**  and choose  **Run \> Run 'sample-headposeestimation'**. As shown in  [Figure 9](#en-us_topic_0219099583_fig93931954162719), the executable program is running on the developer board.

    **Figure  9**  Application running sample<a name="en-us_topic_0219099583_fig93931954162719"></a>  
    

    ![](figures/en-us_image_0219102308.gif)

2.  Use the URL displayed upon the start of the Presenter Server service to log in to Presenter Server. For details, see  [Start Presenter Server](#en-us_topic_0219099583_fig93931954162222).

    Wait for Presenter Agent to transmit data to the server. Click  **Refresh**. When there is data, the icon in the  **Status**  column for the corresponding channel changes to green, as shown in  [Figure 10](#en-us_topic_0219099583_fig113691556202312).

    **Figure  10**  Presenter Server page<a name="en-us_topic_0219099583_fig113691556202312"></a>  
    ![](figures/presenter-server-page-37.png "presenter-server-page-37")

    >![](public_sys-resources/icon-note.gif) **NOTE:**   
    >-   For the head pose estimation application, Presenter Server supports a maximum of 10 channels at the same time \(each  _presenter\_view\_app\_name_  parameter corresponds to a channel\).  
    >-   Due to hardware limitations, each channel supports a maximum frame rate of 20 fps. A lower frame rate is automatically used when the network bandwidth is low.  

3.  Click the link \(such as  **video**  in the preceding figure\) in the  **View Name**  column to view the result. The confidence of the detected head pose is marked.

## Follow-up Operations<a name="section177619345260"></a>

-   Stopping the head pose estimation application

    The head pose estimation application is running continually after being executed. To stop it, perform the following operation:

    Click the stop button shown in  [Figure 11](#en-us_topic_0219099583_fig14326454172518)  to stop the head pose estimation application.

    **Figure  11**  Stopping the head pose estimation application<a name="en-us_topic_0219099583_fig14326454172518"></a>  
    

    ![](figures/en-us_image_0219102592.gif)

    [Figure 12](#en-us_topic_0219099583_fig2182182518112)  shows that the head pose estimation application has been stopped.

    **Figure  12**  Stopped the head pose estimation application<a name="en-us_topic_0219099583_fig2182182518112"></a>  
    

    ![](figures/en-us_image_0219102621.gif)

-   Stopping the Presenter Server service

    The Presenter Server service is always in running state after being started. To stop the Presenter Server service for the head pose estimation application, perform the following operations:

    On the server with  Mind Studio  installed, run the following command as the  Mind Studio  installation user to check the process of the Presenter Server service corresponding to the head pose estimation application:

    **ps -ef | grep presenter | grep headposeestimation**

    ```
    ascend@ascend-HP-ProDesk-600-G4-PCI-MT:~/sample-headposeestimation$ ps -ef | grep presenter | grep headposeestimation 
     ascend    7701  1615  0 14:21 pts/8    00:00:00 python3 presenterserver/presenter_server.py --app Head_pose
    ```

    In the preceding information,  _7701_  indicates the process ID of the Presenter Server service for the head pose estimation application.

    To stop the service, run the following command:

    **kill -9** _7701_



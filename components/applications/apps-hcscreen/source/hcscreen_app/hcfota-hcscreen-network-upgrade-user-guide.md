# 无线同屏器方案网络升级用户指南

无线同屏器SDK支持HTTP网页在线升级和HTTP自动在线升级功能，用户使用此功能时，需要先部署http服务在自己的外网服务器上。

## 1. HTTP网页在线升级

### 1.1 HTTP网页在线升级原理

手机浏览器进入网页加载的时候会自动获取平台应用程序设置的升级配置文件http url，点击网页的升级选项时，网页会自动去parse升级配置文件里的内容，如果有新的版本，会在网页UI上提示最新版本升级，由客户选择升级，客户确认点击升级，网页便会传递升级配置文件里填写的升级固件的http url给平台进行下载烧录。

### 1.2  升级配置文件格式介绍

hcrtos SDK里的components/applications/apps-hcscreen/source/hcscreen_app/目录下存放了一个HCFOTA.jsonp升级配置文件例子。                                 

hclinux SDK的存放位置在 SOURCE/applications/hcscreenapp/hcscreen_app目录下。文件格式内容如下所示。

```
jsonp_callback({
  "product": "HC15A210",
  "version": "2305181630",
  "force_upgrade": true,
  "url": "http://172.16.12.81:80/hccast/rtos/HC15A210/hcscreen/HCFOTA_HC15A210_2305181630.bin"
})
```

1. `product` 字段为defconfig编译文件里定义的名称。如：`BR2_EXTERNAL_PRODUCT_NAME="HC15A210"`。用户可根据自己需求定义自己的product name。

2. `version` 字段为固件的生成时间，要与固件生成的时间一致。

3. `force_upgrade` 字段主要用来控制自动在线升级时检测有新版本是否可以升级，设置为false即不可以升级。

4. `url` 字段为升级固件的http url路径。


### 1.3 平台端HTTP网页在线升级配置步骤

#### 1.3.1 应用程序初始化httpd service

目前hcscreen应用里已经默认有初始化httpd service，客户只需要了解即可，初始化代码例子如下所示。

```
hccast_httpd_service_init(httpd_callback_func);//注册httpd回调函数
hccast_httpd_service_start();//启动httpd服务
```

#### 1.3.2 修改升级配置jsonp文件的url路径

在hcrtos SDK的components/applications/apps-hcscreen/source/hcscreen_app目录下的network_api.c文件里默认定义了Hichip内部的服务器的url，客户需要修改为自己的外部服务器url，并把升级配置文件放在对应的服务器路径上。

hclinux SDK对应为SOURCE/applications/hcscreenapp/hcscreen_app目录下的network_api.c文件。SDK demo里定义升级配置文件url例子如下所示。

```
/**********************************************************************
NETWORK_UPGRADE_URL:
config name: HCFOTA.jsonp
SDK demo url format: server_ip/hccast/os/product_name/app_name/HCFOTA.jsonp
example:http://172.16.12.81:80/hccast/rtos/HC15A210/hcscreen/HCFOTA.jsonp
**********************************************************************/
#ifdef __linux__
#define NETWORK_UPGRADE_URL "http://172.16.12.81:80/hccast/linux/%s/hcscreen/HCFOTA.jsonp"
#else
#define NETWORK_UPGRADE_URL "http://172.16.12.81:80/hccast/rtos/%s/hcscreen/HCFOTA.jsonp"
#endif
```

升级配置文件的url由httpd服务注册的httpd_callback_func回调函数进行获取：

```
static int httpd_callback_func(hccast_httpd_event_e event, void *in, void *out)
{
	app_data_t *app_data = data_mgr_app_get();
	switch (event)
	{
		case HCCAST_HTTPD_GET_UPGRADE_URL:    
		sprintf(in, NETWORK_UPGRADE_URL, sys_data->product_id);
        break;
	}
}
```

#### 1.3.3 放置升级固件到服务器上

用户需要把升级固件文件（HCFOTA_xxx.bin）放在与升级配置文件里url字段对应的http url路径即可。

## 2. HTTP自动在线升级功能

### 2.1 HTTP自动在线升级原理

当平台连上网络后，平台端通过http下载自定义好的升级配置文件url，然后对升级配置文件里的cjson格式内容进行parse，会先判断是否有最新的软件版本，有的话再去判断升级配置文件里的force_upgrade字段是否为true，然后再用升级配置文件里填写的升级固件http url去下载进行烧录升级。

### 2.2 升级配置文件格式介绍

用户可参考本文的1.2章节，自动在线升级配置文件格式与网页在线升级配置格式一致，所以当同时存在自动在线升级和网页在线升级功能，可以使用同一个jsonp文件，也可以分开用两个jsonp文件去做区分。

### 2.3 平台端HTTP自动在线升级配置步骤

#### 2.3.1 使能HTTP自动在线升级功能

hcrtos SDK需要打开applications/apps-hcscreen应用的network_api.c里定义的AUTO_HTTP_UPGRADE宏。hclinux SDK请打开applications/hcscreenapp应用network_api.c里定义的AUTO_HTTP_UPGRADE宏。修改如下所示。

```
//#define AUTO_HTTP_UPGRADE
改为：
#define AUTO_HTTP_UPGRADE
```

#### 2.3.2 修改升级配置jsonp文件的url路径

目前SDK demo里定义的自动在线升级配置文件url和网页在线升级配置文件url一样，由于升级配置文件格式一样，所以都是在network_api.c文件里定义为同一个宏NETWORK_UPGRADE_URL 。用户需要修改为自己的服务器url，并把升级配置文件放置在服务器上。SDK demo里定义升级配置文件url例子如下所示。

```
/**********************************************************************
NETWORK_UPGRADE_URL:
config name: HCFOTA.jsonp
SDK demo url format: server_ip/hccast/os/product_name/app_name/HCFOTA.jsonp
example:http://172.16.12.81:80/hccast/rtos/HC15A210/hcscreen/HCFOTA.jsonp
**********************************************************************/
#ifdef __linux__
#define NETWORK_UPGRADE_URL "http://172.16.12.81:80/hccast/linux/%s/hcscreen/HCFOTA.jsonp"
#else
#define NETWORK_UPGRADE_URL "http://172.16.12.81:80/hccast/rtos/%s/hcscreen/HCFOTA.jsonp"
#endif
```

自动在线升级配置文件的url在代码里调用的地方如下所示。

```
char *network_check_upgrade_url(void)
{
	sys_data_t* sys_data = data_mgr_sys_get();
	snprintf(request_url, sizeof(request_url), NETWORK_UPGRADE_URL, sys_data->product_id);
}
```

#### 2.3.3 放置升级固件到服务器上

用户需要把升级固件文件（HCFOTA_xxx.bin）放在与升级配置文件里url字段对应的http url路径即可。

## 3 常见问题Q&A

### 3.1 点击手机网页在线升级没反应

一般出现此问题的可能原因：

1. 升级配置jsonp文件里的格式填写错误。
2. 升级配置jsonp文件的url填写错误或者该url访问不到，这时候客户可以通过复制url到PC浏览器上看是否可以访问得了。


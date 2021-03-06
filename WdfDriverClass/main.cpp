/************************************************************************/
/* 驱动框架的主入口文件
 此份驱动主要用类来实现wdf驱动框架，方便以后的使用,以后需要用到时则从此类的基础上派生
 子类即可。
*/
/************************************************************************/
#ifdef __cplusplus
extern "C"{
#endif

#include "public.h"
#pragma alloc_text(INIT, DriverEntry)

#ifdef __cplusplus
}
#endif

WdfDriverClass* GetWdfDriverClass();

VOID WdfDriverClassDestroy(IN WDFOBJECT  Object)
{
	PVOID pBuf = WdfMemoryGetBuffer((WDFMEMORY)Object, NULL);		//释放内存资源
	delete pBuf;
}

NTSTATUS WdfDriverClass::DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	WDF_OBJECT_ATTRIBUTES attributes;
	WDF_DRIVER_CONFIG config;

	// 宏内部会调用sizeof(DEVICE_CONTEXT)求结构体长度
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DRIVER_CONTEXT);	//初始化上下文空间

	// 创建WDF框架驱动对象。这个对象我们永远用不到，也不被暴露。但他确实存在着。
	// 我们可以把它看成一只隐藏着的手吧。
	// DriverEntry里面的这种初始化工作，和所有的小端口驱动一模一样，简直别无二致。
	// 读者可以据此参看诸如Scasi、文件过滤、NDIS、StreamPort等类型的小端口驱动，是如何运作的。
	WDF_DRIVER_CONFIG_INIT(&config, WdfDriverClass::PnpAdd_Common);  //注册EvtDriverDeviceAdd事件回调函数,config为返回的被初始化后的结构体
	//config.EvtDriverUnload = ;//设置卸载函数，相当于WDM中DriverUnload函数。一般用于非PNP类驱动中

	NTSTATUS status = WdfDriverCreate(DriverObject, // WDF驱动对象
		RegistryPath, //驱动所对应的服务键在注册表中的路径,信息来自内核配置和管理服务器(Cfg Manager)
		&attributes,//对DriverObject对象的属性配置
		&config, // 配置参数
		&m_hDriver);

	if(NT_SUCCESS(status))
	{
		m_pDriverContext = GetDriverContext(m_hDriver);
		ASSERT(m_pDriverContext);
		m_pDriverContext->par1 = (PVOID)this;	

		WDFMEMORY hMemDrv;
		WDF_OBJECT_ATTRIBUTES_INIT(&attributes);			//初始化结构体
		attributes.ParentObject = m_hDriver;						//设置上层父对象，组成驱动树
		attributes.EvtDestroyCallback  = WdfDriverClassDestroy;
		WdfMemoryCreatePreallocated(&attributes, (PVOID)this, GetSize(), &hMemDrv);//使用框架对类缓冲区进和封装,此方法将保证驱动类的正常工作 WdfObjectDelete将内存对
																																	//象删除后并不释放该内存区域。
	}

	return status;
}


extern "C" NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT  DriverObject, 
	IN PUNICODE_STRING  RegistryPath
	)
{
	WdfDriverClass* myDriver = GetWdfDriverClass();//new(NonPagedPool, 'CY01')DrvClass();
	if(myDriver == NULL)return STATUS_UNSUCCESSFUL;
	return myDriver->DriverEntry(DriverObject, RegistryPath);
}
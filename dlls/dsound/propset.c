/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2002 TransGaming Technologies, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>	/* Insomnia - pow() function */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "winternl.h"
#include "winnls.h"
#include "mmddk.h"
#include "wine/windef16.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"
#include "dsconf.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);


/*******************************************************************************
 *              IKsPropertySet
 */

/* IUnknown methods */
static HRESULT WINAPI IKsPropertySetImpl_QueryInterface(
	LPKSPROPERTYSET iface, REFIID riid, LPVOID *ppobj
) {
	ICOM_THIS(IKsPropertySetImpl,iface);

	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
	return IDirectSoundBuffer_QueryInterface((LPDIRECTSOUNDBUFFER8)This->dsb, riid, ppobj);
}

static ULONG WINAPI IKsPropertySetImpl_AddRef(LPKSPROPERTYSET iface) {
	ICOM_THIS(IKsPropertySetImpl,iface);
	ULONG ulReturn;

	ulReturn = InterlockedIncrement(&This->ref);
	if (ulReturn == 1)
		IDirectSoundBuffer_AddRef((LPDIRECTSOUND3DBUFFER)This->dsb);
	return ulReturn;
}

static ULONG WINAPI IKsPropertySetImpl_Release(LPKSPROPERTYSET iface) {
	ICOM_THIS(IKsPropertySetImpl,iface);
	ULONG ulReturn;

	ulReturn = InterlockedDecrement(&This->ref);
	if (ulReturn)
		return ulReturn;
	IDirectSoundBuffer_Release((LPDIRECTSOUND3DBUFFER)This->dsb);
	return 0;
}

static HRESULT WINAPI DSPROPERTY_WaveDeviceMappingA(
    REFGUID guidPropSet,
    LPVOID pPropData, 
    ULONG cbPropData,
    PULONG pcbReturned )
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A_DATA ppd;
    FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p) not implemented!\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);

    ppd = (PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A_DATA) pPropData;

    if (!ppd) {
	WARN("invalid parameter: pPropData\n");
	return DSERR_INVALIDPARAM;
    }

    FIXME("DeviceName=%s\n",ppd->DeviceName);
    FIXME("DataFlow=%s\n",
	ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER ? "DIRECTSOUNDDEVICE_DATAFLOW_RENDER" :
	ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE ? "DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE" : "UNKNOWN");

    /* FIXME: match the name to a wave device somehow. */
    ppd->DeviceId = GUID_NULL;

    if (pcbReturned) {
	*pcbReturned = cbPropData; 
	FIXME("*pcbReturned=%ld\n", *pcbReturned);
    }

    return S_OK;
}

static HRESULT WINAPI DSPROPERTY_WaveDeviceMappingW(
    REFGUID guidPropSet,
    LPVOID pPropData, 
    ULONG cbPropData,
    PULONG pcbReturned )
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA ppd;
    FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p) not implemented!\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);

    ppd = (PDSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W_DATA) pPropData;

    if (!ppd) {
	WARN("invalid parameter: pPropData\n");
	return DSERR_INVALIDPARAM;
    }

    FIXME("DeviceName=%s\n",debugstr_w(ppd->DeviceName));
    FIXME("DataFlow=%s\n",
	ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER ? "DIRECTSOUNDDEVICE_DATAFLOW_RENDER" :
	ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE ? "DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE" : "UNKNOWN");

    /* FIXME: match the name to a wave device somehow. */
    ppd->DeviceId = GUID_NULL;

    if (pcbReturned) {
	*pcbReturned = cbPropData; 
	FIXME("*pcbReturned=%ld\n", *pcbReturned);
    }

    return S_OK;
}

static HRESULT WINAPI DSPROPERTY_Description1(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    HRESULT err;
    GUID guid, dev_guid;
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA ppd;
    TRACE("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);

    ppd = (PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1_DATA) pPropData;

    if (!ppd) {
	WARN("invalid parameter: pPropData\n");
	return DSERR_INVALIDPARAM;
    }

    TRACE("DeviceId=%s\n",debugstr_guid(&ppd->DeviceId));
    if ( IsEqualGUID( &ppd->DeviceId , &GUID_NULL) ) {
	/* default device of type specified by ppd->DataFlow */
	if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE) {
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE\n");
	} else if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER) {
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
	} else {
	    TRACE("DataFlow=Unknown(%d)\n", ppd->DataFlow);
	}
	FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p) GUID_NULL not implemented!\n",
	    debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);
	return E_PROP_ID_UNSUPPORTED;
    } else {
	GetDeviceID(&ppd->DeviceId, &dev_guid);

	if ( IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultPlayback) || 
	     IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultVoicePlayback) ) {
	    ULONG wod;
	    int wodn;
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
	    ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_RENDER;
	    wodn = waveOutGetNumDevs();
	    for (wod = 0; wod < wodn; wod++) {
		err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDGUID,(DWORD)(&guid),0));
		if (err == DS_OK) {
		    if (IsEqualGUID( &dev_guid, &guid) ) {
			DSDRIVERDESC desc;
			ppd->WaveDeviceId = wod;
			ppd->Devnode = wod;
			err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
			if (err == DS_OK) {
			    strncpy(ppd->DescriptionA, desc.szDesc, sizeof(ppd->DescriptionA) - 1);
			    strncpy(ppd->ModuleA, desc.szDrvName, sizeof(ppd->ModuleA) - 1);
			    MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, ppd->DescriptionW, sizeof(ppd->DescriptionW)/sizeof(WCHAR) );
			    MultiByteToWideChar( CP_ACP, 0, desc.szDrvName, -1, ppd->ModuleW, sizeof(ppd->ModuleW)/sizeof(WCHAR) );
			    break;
			} else {
			    WARN("waveOutMessage failed\n");
			    return E_PROP_ID_UNSUPPORTED;
			}
		    }
		} else {
		    WARN("waveOutMessage failed\n");
		    return E_PROP_ID_UNSUPPORTED;
		}
	    }
	} else if ( IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultCapture) ||
	            IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultVoiceCapture) ) {
	    ULONG wid;
	    int widn;
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE\n");
	    ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE;
	    widn = waveInGetNumDevs();
	    for (wid = 0; wid < widn; wid++) {
		err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDGUID,(DWORD)(&guid),0));
		if (err == DS_OK) {
		    if (IsEqualGUID( &dev_guid, &guid) ) {
			DSDRIVERDESC desc;
			ppd->WaveDeviceId = wid;
			ppd->Devnode = wid;
			err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
			if (err == DS_OK) {
			    strncpy(ppd->DescriptionA, desc.szDesc, sizeof(ppd->DescriptionA) - 1);
			    strncpy(ppd->ModuleA, desc.szDrvName, sizeof(ppd->ModuleA) - 1);
			    MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, ppd->DescriptionW, sizeof(ppd->DescriptionW)/sizeof(WCHAR) );
			    MultiByteToWideChar( CP_ACP, 0, desc.szDrvName, -1, ppd->ModuleW, sizeof(ppd->ModuleW)/sizeof(WCHAR) );
			    break;
			} else {
			    WARN("waveInMessage failed\n");
			    return E_PROP_ID_UNSUPPORTED;
			}
			break;
		    }
		} else {
		    WARN("waveInMessage failed\n");
		    return E_PROP_ID_UNSUPPORTED;
		}
	    }
	} else {
	    FIXME("DeviceId=Unknown\n");
	    return E_PROP_ID_UNSUPPORTED;
	}
    }

    ppd->Type = DIRECTSOUNDDEVICE_TYPE_EMULATED;

    if (pcbReturned) {
	*pcbReturned = cbPropData; 
	TRACE("*pcbReturned=%ld\n", *pcbReturned);
    }

    return S_OK;
}

static HRESULT WINAPI DSPROPERTY_DescriptionA(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);
    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI DSPROPERTY_DescriptionW(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA ppd = (PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W_DATA) pPropData;
    HRESULT err;
    GUID guid, dev_guid;
    TRACE("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);

    TRACE("DeviceId=%s\n",debugstr_guid(&ppd->DeviceId));
    if ( IsEqualGUID( &ppd->DeviceId , &GUID_NULL) ) {
	/* default device of type specified by ppd->DataFlow */
	if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE) {
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE\n");
	} else if (ppd->DataFlow == DIRECTSOUNDDEVICE_DATAFLOW_RENDER) {
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
	} else {
	    TRACE("DataFlow=Unknown(%d)\n", ppd->DataFlow);
	}
	FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p) GUID_NULL not implemented!\n",
	    debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);
	return E_PROP_ID_UNSUPPORTED;
    } else {
	GetDeviceID(&ppd->DeviceId, &dev_guid);

	if ( IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultPlayback) || 
	     IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultVoicePlayback) ) {
	    ULONG wod;
	    int wodn;
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_RENDER\n");
	    ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_RENDER;
	    wodn = waveOutGetNumDevs();
	    for (wod = 0; wod < wodn; wod++) {
		err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDGUID,(DWORD)(&guid),0));
		if (err == DS_OK) {
		    if (IsEqualGUID( &dev_guid, &guid) ) {
			DSDRIVERDESC desc;
			ppd->WaveDeviceId = wod;
			err = mmErr(waveOutMessage((HWAVEOUT)wod,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
			if (err == DS_OK) {
			    /* FIXME: this is a memory leak */
			    WCHAR * wDescription = HeapAlloc(GetProcessHeap(),0,0x200);
			    WCHAR * wModule = HeapAlloc(GetProcessHeap(),0,0x200);
			    WCHAR * wInterface = HeapAlloc(GetProcessHeap(),0,0x200);

			    MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, wDescription, 0x100  );
			    MultiByteToWideChar( CP_ACP, 0, desc.szDrvName, -1, wModule, 0x100 );
			    MultiByteToWideChar( CP_ACP, 0, "Interface", -1, wInterface, 0x100 );

			    ppd->Description = wDescription;
			    ppd->Module = wModule;
			    ppd->Interface = wInterface;
			    break;
			} else {
			    WARN("waveOutMessage failed\n");
			    return E_PROP_ID_UNSUPPORTED;
			}
		    }
		} else {
		    WARN("waveOutMessage failed\n");
		    return E_PROP_ID_UNSUPPORTED;
		}
	    }
	} else if (IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultCapture) ||
		   IsEqualGUID( &ppd->DeviceId , &DSDEVID_DefaultVoiceCapture) ) {
	    ULONG wid;
	    int widn;
	    TRACE("DataFlow=DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE\n");
	    ppd->DataFlow = DIRECTSOUNDDEVICE_DATAFLOW_CAPTURE;
	    widn = waveInGetNumDevs();
	    for (wid = 0; wid < widn; wid++) {
		err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDGUID,(DWORD)(&guid),0));
		if (err == DS_OK) {
		    if (IsEqualGUID( &dev_guid, &guid) ) {
			DSDRIVERDESC desc;
			ppd->WaveDeviceId = wid;
			err = mmErr(waveInMessage((HWAVEIN)wid,DRV_QUERYDSOUNDDESC,(DWORD)&(desc),0));
			if (err == DS_OK) {
			    /* FIXME: this is a memory leak */
			    WCHAR * wDescription = HeapAlloc(GetProcessHeap(),0,0x200);
			    WCHAR * wModule = HeapAlloc(GetProcessHeap(),0,0x200);
			    WCHAR * wInterface = HeapAlloc(GetProcessHeap(),0,0x200);

			    MultiByteToWideChar( CP_ACP, 0, desc.szDesc, -1, wDescription, 0x100  );
			    MultiByteToWideChar( CP_ACP, 0, desc.szDrvName, -1, wModule, 0x100 );
			    MultiByteToWideChar( CP_ACP, 0, "Interface", -1, wInterface, 0x100 );

			    ppd->Description = wDescription;
			    ppd->Module = wModule;
			    ppd->Interface = wInterface;
			    break;
			} else {
			    WARN("waveInMessage failed\n");
			    return E_PROP_ID_UNSUPPORTED;
			}
			break;
		    }
		} else {
		    WARN("waveInMessage failed\n");
		    return E_PROP_ID_UNSUPPORTED;
		}
	    }
	} else {
	    FIXME("DeviceId=Unknown\n");
	    return E_PROP_ID_UNSUPPORTED;
	}
    }

    ppd->Type = DIRECTSOUNDDEVICE_TYPE_EMULATED;
    if (pcbReturned) {
	*pcbReturned = cbPropData; 
	TRACE("*pcbReturned=%ld\n", *pcbReturned);
    }
    return S_OK;
}

static HRESULT WINAPI DSPROPERTY_Enumerate1(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);
    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI DSPROPERTY_EnumerateA(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);
    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI DSPROPERTY_EnumerateW(
    REFGUID guidPropSet,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    FIXME("(guidPropSet=%s,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
	debugstr_guid(guidPropSet),pPropData,cbPropData,pcbReturned);
    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsPropertySetImpl_Get(LPKSPROPERTYSET iface,
	REFGUID guidPropSet, ULONG dwPropID,
	LPVOID pInstanceData, ULONG cbInstanceData,
	LPVOID pPropData, ULONG cbPropData,
	PULONG pcbReturned
) {
	ICOM_THIS(IKsPropertySetImpl,iface);
	TRACE("(iface=%p,guidPropSet=%s,dwPropID=%ld,pInstanceData=%p,cbInstanceData=%ld,pPropData=%p,cbPropData=%ld,pcbReturned=%p)\n",
		This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData,pcbReturned);

	if ( IsEqualGUID( &DSPROPSETID_DirectSoundDevice, guidPropSet) ) {
		switch (dwPropID) {
		case DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_A:
			return DSPROPERTY_WaveDeviceMappingA(guidPropSet,pPropData,cbPropData,pcbReturned);
		case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_1:
			return DSPROPERTY_Description1(guidPropSet,pPropData,cbPropData,pcbReturned);
		case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_1:
			return DSPROPERTY_Enumerate1(guidPropSet,pPropData,cbPropData,pcbReturned);
		case DSPROPERTY_DIRECTSOUNDDEVICE_WAVEDEVICEMAPPING_W:
			return DSPROPERTY_WaveDeviceMappingW(guidPropSet,pPropData,cbPropData,pcbReturned);
		case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_A:
			return DSPROPERTY_DescriptionA(guidPropSet,pPropData,cbPropData,pcbReturned);
		case DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_W:
			return DSPROPERTY_DescriptionW(guidPropSet,pPropData,cbPropData,pcbReturned);
		case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_A:
			return DSPROPERTY_EnumerateA(guidPropSet,pPropData,cbPropData,pcbReturned);
		case DSPROPERTY_DIRECTSOUNDDEVICE_ENUMERATE_W:
			return DSPROPERTY_EnumerateW(guidPropSet,pPropData,cbPropData,pcbReturned);
		default:
			FIXME("unsupported ID: %ld\n",dwPropID);
			break;
		}
	} else {
		FIXME("unsupported property: %s\n",debugstr_guid(guidPropSet));
	}

	if (pcbReturned) {
		*pcbReturned = 0; 
		FIXME("*pcbReturned=%ld\n", *pcbReturned);
	}

	return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsPropertySetImpl_Set(LPKSPROPERTYSET iface,
	REFGUID guidPropSet, ULONG dwPropID,
	LPVOID pInstanceData, ULONG cbInstanceData,
	LPVOID pPropData, ULONG cbPropData
) {
	ICOM_THIS(IKsPropertySetImpl,iface);

	FIXME("(%p,%s,%ld,%p,%ld,%p,%ld), stub!\n",This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData);
	return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsPropertySetImpl_QuerySupport(LPKSPROPERTYSET iface,
	REFGUID guidPropSet, ULONG dwPropID, PULONG pTypeSupport
) {
	ICOM_THIS(IKsPropertySetImpl,iface);

	FIXME("(%p,%s,%ld,%p), stub!\n",This,debugstr_guid(guidPropSet),dwPropID,pTypeSupport);
	return E_PROP_ID_UNSUPPORTED;
}

static ICOM_VTABLE(IKsPropertySet) iksvt = {
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IKsPropertySetImpl_QueryInterface,
	IKsPropertySetImpl_AddRef,
	IKsPropertySetImpl_Release,
	IKsPropertySetImpl_Get,
	IKsPropertySetImpl_Set,
	IKsPropertySetImpl_QuerySupport
};

HRESULT WINAPI IKsPropertySetImpl_Create(
	IDirectSoundBufferImpl *This,
	IKsPropertySetImpl **piks)
{
	IKsPropertySetImpl *iks;

	iks = (IKsPropertySetImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(*iks));
	iks->ref = 0;
	iks->dsb = This;
	iks->lpVtbl = &iksvt;

	*piks = iks;
	return S_OK;
}

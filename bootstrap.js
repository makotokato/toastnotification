/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/ctypes.jsm");

var sLibrary;

function IsSupported()
{
  if (!sLibrary) {
  	if (ctypes.voidptr_t.size == 4) {
      sLibrary = ctypes.open("toast32.dll");
    } else {
      sLibrary = ctypes.open("toast64.dll");
    }
  }

  var setAppId = sLibrary.declare("SetAppId",
                                 ctypes.winapi_abi,
                                 ctypes.bool);
  return setAppId();
}

function ToastAlertService()
{
}

ToastAlertService.prototype = {
  showAlertNotification: function(aImageUrl, aTitle, aText, aTextClickable,
                                  aCookie, aListener, aName, aDir, aLang,
                                  aPrincipal) {
  	if (ctypes.voidptr_t.size == 4) {
      this._library = ctypes.open("toast32.dll");
    } else {
      this._library = ctypes.open("toast64.dll");
    } 

    var DisplayToastNotification = this._library.declare("DisplayToastNotification",
                                     ctypes.winapi_abi,
                                     ctypes.bool,
                                     ctypes.jschar.ptr,
                                     ctypes.jschar.ptr,
                                     ctypes.jschar.ptr,
                                     ctypes.FunctionType(ctypes.winapi_abi,
                                                         ctypes.void_t),
                                     ctypes.FunctionType(ctypes.winapi_abi,
                                                         ctypes.void_t));


    if (!DisplayToastNotification(aTitle, aText, aName,
                                  () => {
                   	                if (aListener && aTextClickable) {
                   	                  aListener.observe(null, "alertclickcallback", aCookie);
                                    }
                                  },
                                  () => {
                   	                if (aListener) {
                                      aListener.observe(null, "alertfinished", aCookie);
                                    }
                                    this._library.close();
                                    this._library = null;
                                  })) {
    	throw Cr.NS_ERROR_FAILURE;
    }

    if (aListener) {
      aListener.observe(null, "alertshow", aCookie);
    }
  },

  closeAlert: function(aName, aPrincipal) {
  	if (!this._library) {
  	  return;
  	}
    var CloseToastNotification = this._library.declare("CloseToastNotification",
                                     ctypes.winapi_abi,
                                     ctypes.bool,
                                     ctypes.jschar.ptr);
    CloseToastNotification(aName);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAlertService]),

  classDescription: "alert service using Windows 8 Toast",
  contractID: "@mozilla.org/system-alerts-service;1",
  classID: Components.ID("{}"),

  _library: null,
};

var alertService = new ToastAlertService;

var factory = {
  createInstance: function(aOuter, aIid) {
    if (aOuter) {
      throw Cr.NS_ERROR_NO_AGGREGATION;
    }
    return alertService.QueryInterface(aIid);
  },
};

function registerComponents()
{
  if (!IsSupported()) {
    return;
  }

  let registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
  registrar.registerFactory(alertService.classID,
                            alertService.classDescription,
                            alertService.contractID,
                            factory);
}

function unregisterComponents()
{
  if (sLibrary) {
  	sLibrary.close();
    sLibrary = null;
  }
  try {
    let registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
    registrar.unregisterFactory(alertService.classID, factory);
  } catch (e) {
  	// ignore error
  }
}

functioe install(aData, aReason)
{
  registerComponents();
}

function uninstall(aData, aReason)
{
  unregisterComponents();
}
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;
const Cm = Components.manager;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/ctypes.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");

var sLibrary;
var sCallbacks = new Array();

function GetBinaryFile()
{
  if (ctypes.voidptr_t.size == 4) {
    return FileUtils.getFile("ProfD", ["extensions", "toast-alert@wontfix.net", "ToastNotification.dll"]);
  }
  return FileUtils.getFile("ProfD", ["extensions", "toast-alert@wontfix.net", "ToastNotification64.dll"]);
}

function IsSupported()
{
  if (!sLibrary) {
    sLibrary = ctypes.open(GetBinaryFile().path);
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
                                  aData, aPrincipal, aInPrivateBrowsing) {
    var callbackPtr = ctypes.FunctionType(ctypes.stdcall_abi, ctypes.void_t).ptr;

    var DisplayToastNotification = sLibrary.declare("DisplayToastNotification",
                                     ctypes.winapi_abi,
                                     ctypes.bool,
                                     ctypes.jschar.ptr,
                                     ctypes.jschar.ptr,
                                     ctypes.jschar.ptr,
                                     callbackPtr,
                                     callbackPtr);

    var callbackClick = null;
    if (aTextClickable) {
      callbackClick = callbackPtr(function() {
        if (this.listener) {
          this.listener.observe(null, "alertclickcallback", this.cookie);
        }
      },
      { listener: aListener, cookie: aCookie });
    }

    var callbackClose = callbackPtr(function() {
      if (this.listener) {
        this.listener.observe(null, "alertfinished", this.cookie);
      }
      sCallbacks.forEach(function(element) {
        if (element.name === this.name) {
          sCallbacks.pop(element);
        }      
      });
    },
    { listener: aListener, cookie: aCookie, name: aName });

    // keep callback objects for GC
    sCallbacks.push({click: callbackClick, close: callbackClose, name: aName});

    if (!DisplayToastNotification(aTitle, aText, aName, callbackClick, callbackClose)) {
    	throw Cr.NS_ERROR_FAILURE;
    }

    if (aListener) {
      aListener.observe(null, "alertshow", aCookie);
    }
  },

  closeAlert: function(aName, aPrincipal) {
    var CloseToastNotification = sLibrary.declare("CloseToastNotification",
                                     ctypes.winapi_abi,
                                     ctypes.bool,
                                     ctypes.jschar.ptr);
    CloseToastNotification(aName);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAlertsService]),

  classDescription: "alert service using Windows 8 Toast",
  contractID: "@mozilla.org/system-alerts-service;1",
  classID: Components.ID("{6270a80b-07e3-481b-804a-644ed6bc991d}"),
};

var alertService = new ToastAlertService;

var factory = {
  createInstance: function(aOuter, aIid) {
    if (aOuter) {
      throw Cr.NS_ERROR_NO_AGGREGATION;
    }
    return alertService.QueryInterface(aIid);
  },
  lockFactory: function(aLock) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFactory])
};

function registerComponents()
{
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

function install(aData, aReason)
{
}

function uninstall(aData, aReason)
{
}

function startup(aData, aReason)
{
  if (!IsSupported()) {
    return;
  }
  registerComponents();
}

function shutdown(aData, aReason)
{
  unregisterComponents();
}

/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.0
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

namespace OpenTransactions.OTAPI {

public class OTNameLookup : global::System.IDisposable {
  private global::System.Runtime.InteropServices.HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal OTNameLookup(global::System.IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new global::System.Runtime.InteropServices.HandleRef(this, cPtr);
  }

  internal static global::System.Runtime.InteropServices.HandleRef getCPtr(OTNameLookup obj) {
    return (obj == null) ? new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero) : obj.swigCPtr;
  }

  ~OTNameLookup() {
    Dispose();
  }

  public virtual void Dispose() {
    lock(this) {
      if (swigCPtr.Handle != global::System.IntPtr.Zero) {
        if (swigCMemOwn) {
          swigCMemOwn = false;
          otapiPINVOKE.delete_OTNameLookup(swigCPtr);
        }
        swigCPtr = new global::System.Runtime.InteropServices.HandleRef(null, global::System.IntPtr.Zero);
      }
      global::System.GC.SuppressFinalize(this);
    }
  }

  public OTNameLookup() : this(otapiPINVOKE.new_OTNameLookup(), true) {
    SwigDirectorConnect();
  }

  public virtual string GetNymName(string str_id, SWIGTYPE_p_std__string p_server_id) {
    string ret = (SwigDerivedClassHasMethod("GetNymName", swigMethodTypes0) ? otapiPINVOKE.OTNameLookup_GetNymNameSwigExplicitOTNameLookup__SWIG_0(swigCPtr, str_id, SWIGTYPE_p_std__string.getCPtr(p_server_id)) : otapiPINVOKE.OTNameLookup_GetNymName__SWIG_0(swigCPtr, str_id, SWIGTYPE_p_std__string.getCPtr(p_server_id)));
    if (otapiPINVOKE.SWIGPendingException.Pending) throw otapiPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public virtual string GetNymName(string str_id) {
    string ret = (SwigDerivedClassHasMethod("GetNymName", swigMethodTypes1) ? otapiPINVOKE.OTNameLookup_GetNymNameSwigExplicitOTNameLookup__SWIG_1(swigCPtr, str_id) : otapiPINVOKE.OTNameLookup_GetNymName__SWIG_1(swigCPtr, str_id));
    if (otapiPINVOKE.SWIGPendingException.Pending) throw otapiPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public virtual string GetAcctName(string str_id, SWIGTYPE_p_std__string p_nym_id, SWIGTYPE_p_std__string p_server_id, SWIGTYPE_p_std__string p_asset_id) {
    string ret = (SwigDerivedClassHasMethod("GetAcctName", swigMethodTypes2) ? otapiPINVOKE.OTNameLookup_GetAcctNameSwigExplicitOTNameLookup__SWIG_0(swigCPtr, str_id, SWIGTYPE_p_std__string.getCPtr(p_nym_id), SWIGTYPE_p_std__string.getCPtr(p_server_id), SWIGTYPE_p_std__string.getCPtr(p_asset_id)) : otapiPINVOKE.OTNameLookup_GetAcctName__SWIG_0(swigCPtr, str_id, SWIGTYPE_p_std__string.getCPtr(p_nym_id), SWIGTYPE_p_std__string.getCPtr(p_server_id), SWIGTYPE_p_std__string.getCPtr(p_asset_id)));
    if (otapiPINVOKE.SWIGPendingException.Pending) throw otapiPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public virtual string GetAcctName(string str_id, SWIGTYPE_p_std__string p_nym_id, SWIGTYPE_p_std__string p_server_id) {
    string ret = (SwigDerivedClassHasMethod("GetAcctName", swigMethodTypes3) ? otapiPINVOKE.OTNameLookup_GetAcctNameSwigExplicitOTNameLookup__SWIG_1(swigCPtr, str_id, SWIGTYPE_p_std__string.getCPtr(p_nym_id), SWIGTYPE_p_std__string.getCPtr(p_server_id)) : otapiPINVOKE.OTNameLookup_GetAcctName__SWIG_1(swigCPtr, str_id, SWIGTYPE_p_std__string.getCPtr(p_nym_id), SWIGTYPE_p_std__string.getCPtr(p_server_id)));
    if (otapiPINVOKE.SWIGPendingException.Pending) throw otapiPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public virtual string GetAcctName(string str_id, SWIGTYPE_p_std__string p_nym_id) {
    string ret = (SwigDerivedClassHasMethod("GetAcctName", swigMethodTypes4) ? otapiPINVOKE.OTNameLookup_GetAcctNameSwigExplicitOTNameLookup__SWIG_2(swigCPtr, str_id, SWIGTYPE_p_std__string.getCPtr(p_nym_id)) : otapiPINVOKE.OTNameLookup_GetAcctName__SWIG_2(swigCPtr, str_id, SWIGTYPE_p_std__string.getCPtr(p_nym_id)));
    if (otapiPINVOKE.SWIGPendingException.Pending) throw otapiPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public virtual string GetAcctName(string str_id) {
    string ret = (SwigDerivedClassHasMethod("GetAcctName", swigMethodTypes5) ? otapiPINVOKE.OTNameLookup_GetAcctNameSwigExplicitOTNameLookup__SWIG_3(swigCPtr, str_id) : otapiPINVOKE.OTNameLookup_GetAcctName__SWIG_3(swigCPtr, str_id));
    if (otapiPINVOKE.SWIGPendingException.Pending) throw otapiPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  public virtual string GetAddressName(string str_address) {
    string ret = (SwigDerivedClassHasMethod("GetAddressName", swigMethodTypes6) ? otapiPINVOKE.OTNameLookup_GetAddressNameSwigExplicitOTNameLookup(swigCPtr, str_address) : otapiPINVOKE.OTNameLookup_GetAddressName(swigCPtr, str_address));
    if (otapiPINVOKE.SWIGPendingException.Pending) throw otapiPINVOKE.SWIGPendingException.Retrieve();
    return ret;
  }

  private void SwigDirectorConnect() {
    if (SwigDerivedClassHasMethod("GetNymName", swigMethodTypes0))
      swigDelegate0 = new SwigDelegateOTNameLookup_0(SwigDirectorGetNymName__SWIG_0);
    if (SwigDerivedClassHasMethod("GetNymName", swigMethodTypes1))
      swigDelegate1 = new SwigDelegateOTNameLookup_1(SwigDirectorGetNymName__SWIG_1);
    if (SwigDerivedClassHasMethod("GetAcctName", swigMethodTypes2))
      swigDelegate2 = new SwigDelegateOTNameLookup_2(SwigDirectorGetAcctName__SWIG_0);
    if (SwigDerivedClassHasMethod("GetAcctName", swigMethodTypes3))
      swigDelegate3 = new SwigDelegateOTNameLookup_3(SwigDirectorGetAcctName__SWIG_1);
    if (SwigDerivedClassHasMethod("GetAcctName", swigMethodTypes4))
      swigDelegate4 = new SwigDelegateOTNameLookup_4(SwigDirectorGetAcctName__SWIG_2);
    if (SwigDerivedClassHasMethod("GetAcctName", swigMethodTypes5))
      swigDelegate5 = new SwigDelegateOTNameLookup_5(SwigDirectorGetAcctName__SWIG_3);
    if (SwigDerivedClassHasMethod("GetAddressName", swigMethodTypes6))
      swigDelegate6 = new SwigDelegateOTNameLookup_6(SwigDirectorGetAddressName);
    otapiPINVOKE.OTNameLookup_director_connect(swigCPtr, swigDelegate0, swigDelegate1, swigDelegate2, swigDelegate3, swigDelegate4, swigDelegate5, swigDelegate6);
  }

  private bool SwigDerivedClassHasMethod(string methodName, global::System.Type[] methodTypes) {
    global::System.Reflection.MethodInfo methodInfo = this.GetType().GetMethod(methodName, global::System.Reflection.BindingFlags.Public | global::System.Reflection.BindingFlags.NonPublic | global::System.Reflection.BindingFlags.Instance, null, methodTypes, null);
    bool hasDerivedMethod = methodInfo.DeclaringType.IsSubclassOf(typeof(OTNameLookup));
    return hasDerivedMethod;
  }

  private string SwigDirectorGetNymName__SWIG_0(string str_id, global::System.IntPtr p_server_id) {
    return GetNymName(str_id, (p_server_id == global::System.IntPtr.Zero) ? null : new SWIGTYPE_p_std__string(p_server_id, false));
  }

  private string SwigDirectorGetNymName__SWIG_1(string str_id) {
    return GetNymName(str_id);
  }

  private string SwigDirectorGetAcctName__SWIG_0(string str_id, global::System.IntPtr p_nym_id, global::System.IntPtr p_server_id, global::System.IntPtr p_asset_id) {
    return GetAcctName(str_id, (p_nym_id == global::System.IntPtr.Zero) ? null : new SWIGTYPE_p_std__string(p_nym_id, false), (p_server_id == global::System.IntPtr.Zero) ? null : new SWIGTYPE_p_std__string(p_server_id, false), (p_asset_id == global::System.IntPtr.Zero) ? null : new SWIGTYPE_p_std__string(p_asset_id, false));
  }

  private string SwigDirectorGetAcctName__SWIG_1(string str_id, global::System.IntPtr p_nym_id, global::System.IntPtr p_server_id) {
    return GetAcctName(str_id, (p_nym_id == global::System.IntPtr.Zero) ? null : new SWIGTYPE_p_std__string(p_nym_id, false), (p_server_id == global::System.IntPtr.Zero) ? null : new SWIGTYPE_p_std__string(p_server_id, false));
  }

  private string SwigDirectorGetAcctName__SWIG_2(string str_id, global::System.IntPtr p_nym_id) {
    return GetAcctName(str_id, (p_nym_id == global::System.IntPtr.Zero) ? null : new SWIGTYPE_p_std__string(p_nym_id, false));
  }

  private string SwigDirectorGetAcctName__SWIG_3(string str_id) {
    return GetAcctName(str_id);
  }

  private string SwigDirectorGetAddressName(string str_address) {
    return GetAddressName(str_address);
  }

  public delegate string SwigDelegateOTNameLookup_0(string str_id, global::System.IntPtr p_server_id);
  public delegate string SwigDelegateOTNameLookup_1(string str_id);
  public delegate string SwigDelegateOTNameLookup_2(string str_id, global::System.IntPtr p_nym_id, global::System.IntPtr p_server_id, global::System.IntPtr p_asset_id);
  public delegate string SwigDelegateOTNameLookup_3(string str_id, global::System.IntPtr p_nym_id, global::System.IntPtr p_server_id);
  public delegate string SwigDelegateOTNameLookup_4(string str_id, global::System.IntPtr p_nym_id);
  public delegate string SwigDelegateOTNameLookup_5(string str_id);
  public delegate string SwigDelegateOTNameLookup_6(string str_address);

  private SwigDelegateOTNameLookup_0 swigDelegate0;
  private SwigDelegateOTNameLookup_1 swigDelegate1;
  private SwigDelegateOTNameLookup_2 swigDelegate2;
  private SwigDelegateOTNameLookup_3 swigDelegate3;
  private SwigDelegateOTNameLookup_4 swigDelegate4;
  private SwigDelegateOTNameLookup_5 swigDelegate5;
  private SwigDelegateOTNameLookup_6 swigDelegate6;

  private static global::System.Type[] swigMethodTypes0 = new global::System.Type[] { typeof(string), typeof(SWIGTYPE_p_std__string) };
  private static global::System.Type[] swigMethodTypes1 = new global::System.Type[] { typeof(string) };
  private static global::System.Type[] swigMethodTypes2 = new global::System.Type[] { typeof(string), typeof(SWIGTYPE_p_std__string), typeof(SWIGTYPE_p_std__string), typeof(SWIGTYPE_p_std__string) };
  private static global::System.Type[] swigMethodTypes3 = new global::System.Type[] { typeof(string), typeof(SWIGTYPE_p_std__string), typeof(SWIGTYPE_p_std__string) };
  private static global::System.Type[] swigMethodTypes4 = new global::System.Type[] { typeof(string), typeof(SWIGTYPE_p_std__string) };
  private static global::System.Type[] swigMethodTypes5 = new global::System.Type[] { typeof(string) };
  private static global::System.Type[] swigMethodTypes6 = new global::System.Type[] { typeof(string) };
}

}
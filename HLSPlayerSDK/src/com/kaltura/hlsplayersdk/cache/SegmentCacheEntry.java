package com.kaltura.hlsplayersdk.cache;

import android.os.Handler;
import android.util.Log;

import com.loopj.android.http.*;

public class SegmentCacheEntry {
	public String uri;
	public byte[] data;
	public boolean running;
	public long lastTouchedMillis;
	public long forceSize = -1;

	// If >= 0, ID of a crypto context on the native side.
	protected int cryptoHandle = -1;

	// All bytes < decryptHighWaterMark are descrypted; all >=  are still 
	// encrypted. This allows us to avoid duplicating every segment.
	protected long decryptHighWaterMark = 0;
	
	// We will retry 3 times before giving up
	private static final int maxRetries = 3;
	private int curRetries = 0;
	
	
	public static native int allocAESCryptoState(byte[] key, byte[] iv);
	public static native void freeCryptoState(int id);
	public static native long decrypt(int cryptoHandle, byte[] data, long start, long length);
	
	public RequestHandle request = null;
	
	private Handler mCallbackHandler = null;

	public boolean hasCrypto()
	{
		return (cryptoHandle != -1);
	}

	public void setCryptoHandle(int handle)
	{
		cryptoHandle = handle;
	}

	public void ensureDecryptedTo(long offset)
	{
		if(cryptoHandle == -1)
			return;
		
		//Log.i("HLS Cache", "Decrypting to " + offset);
		//Log.i("HLS Cache", "  first byte = " + data[0]);
		long delta = offset - decryptHighWaterMark;
		//Log.i("HLS Cache", "  delta = " + delta + " | HighWaterMark = " + decryptHighWaterMark);
		if (delta > 0)
			decryptHighWaterMark = decrypt(cryptoHandle, data, decryptHighWaterMark, delta);
		//Log.i("HLS Cache", "Decrypted to " + decryptHighWaterMark);
		//Log.i("HLS Cache", "  first byte = " + data[0]);
	}

	public boolean isFullyDecrypted()
	{
		return (decryptHighWaterMark == data.length);
	}
	
	private SegmentCachedListener mSegmentCachedListener = null;
	public void registerSegmentCachedListener(SegmentCachedListener listener, Handler callbackHandler)
	{
		if (mSegmentCachedListener != listener)
		{
			Log.i("SegmentCacheEntry", "Setting the SegmentCachedListener to a new value: " + listener);
			mSegmentCachedListener = listener;
			mCallbackHandler = callbackHandler;
		}
	}
	
	public void notifySegmentCached()
	{
		if (mSegmentCachedListener != null && mCallbackHandler != null)
		{
			mCallbackHandler.post(new Runnable() {
				public void run()
				{
					mSegmentCachedListener.onSegmentCompleted(uri);
				}
			});
		}
	}
	
	private boolean retry()
	{
		++curRetries;
		if (curRetries >= maxRetries) return false;
		return true;
	}
	
	public void postOnSegmentFailed(int statusCode)
	{
		if (mSegmentCachedListener != null)
			mSegmentCachedListener.onSegmentFailed(uri, statusCode);
	}
	
	public void postSegmentSucceeded(int statusCode, byte[] responseData)
	{
		if (statusCode == 200)
		{
			Log.i("HLS Cache", "Got " + uri);
			HLSSegmentCache.store(uri, responseData);
		}
		else
		{
			if (mSegmentCachedListener != null)
				mSegmentCachedListener.onSegmentFailed(uri, statusCode);
		}
	}

}

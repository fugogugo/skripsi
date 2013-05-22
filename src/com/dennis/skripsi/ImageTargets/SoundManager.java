/*==============================================================================
            Copyright (c) 2012 QUALCOMM Austria Research Center GmbH.
            All Rights Reserved.
            Qualcomm Confidential and Proprietary
==============================================================================*/

package com.dennis.skripsi.ImageTargets;

import java.util.ArrayList;

import android.content.Context;
import android.media.AudioManager;
import android.media.SoundPool;

public class SoundManager {

    // SoundPool is used for short sound effects
    private SoundPool mSoundPool;
    
    // An array of sound clip ids
    public ArrayList<Integer> mSoundIds;
    
    
    /** Initialize the sound pool and load our sound clips. */
    public SoundManager(Context context)
    {
        mSoundPool = new SoundPool(4, AudioManager.STREAM_MUSIC, 0);
        mSoundIds = new ArrayList<Integer>();
        
        mSoundIds.add(mSoundPool.load(context, R.raw.kuda,1));
        mSoundIds.add(mSoundPool.load(context, R.raw.anjing, 1));
        mSoundIds.add(mSoundPool.load(context, R.raw.kucing, 1));
        mSoundIds.add(mSoundPool.load(context, R.raw.sapi, 1));
        mSoundIds.add(mSoundPool.load(context, R.raw.ikan, 1));
        mSoundIds.add(mSoundPool.load(context, R.raw.babi, 1));
        mSoundIds.add(mSoundPool.load(context, R.raw.harimau, 1));
//        mSoundPool.load(context.getAssets().openFd(""), 1);
    }
    
    
    /** Play a sound with the given index and volume. */
    public void playSound(int soundIndex, float volume)
    {
        if (soundIndex < 0 || soundIndex >= mSoundIds.size()) {
            DebugLog.LOGE("Sound id " + soundIndex + " out of range");
            return;
        }
        
        mSoundPool.play(mSoundIds.get(soundIndex), volume, volume, 1, 0, 1.0f);
    }
}

/******************************************************************************
 * Copyright (c) 2014, AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

package org.alljoyn.ns.transport;

import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.RejectedExecutionHandler;
import java.util.concurrent.SynchronousQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import org.alljoyn.ns.NotificationService;
import org.alljoyn.ns.commons.NativePlatform;

/**
 *  The thread pool manager, is used to manage a different {@link NotificationService} tasks.
 */
class WorkersPoolManager implements RejectedExecutionHandler {
	private static final String TAG = "ioe" + WorkersPoolManager.class.getSimpleName();
	
	/**
	 * The platform dependent object
	 */
	private NativePlatform nativePlatform;
	
	/**
	 * The thread TTL time
	 */
	private static final long THREAD_TTL        = 3;
	
	/**
	 * The thread TTL time unit
	 */
	private static final TimeUnit TTL_TIME_UNIT =  TimeUnit.SECONDS;
	
	/**
	 * Amount of the core threads 
	 */
	private static final int THREAD_CORE_NUM    = 4;
	
	/**
	 * The maximum number of threads that may be spawn
	 */
	private static final int THREAD_MAX_NUM     = 10;
	
	/**
	 * Thread pool object
	 */
	private ThreadPoolExecutor threadPool;

	/**
	 * Constructor 
	 * Starts the thread pool
	 * @param nativePlatform
	 */
	public WorkersPoolManager(NativePlatform nativePlatform) {
		this.nativePlatform = nativePlatform;
		initPool();
	}

	/**
	 * @param task Execute the given runnable task if there is a free thread worker
	 */
	public void execute(Runnable task) {
		
		nativePlatform.getNativeLogger().debug(TAG,"Executing task, Current Thread pool size: '" +
												    threadPool.getPoolSize() + "', 'Number of currently working threads: '" +
												    threadPool.getActiveCount() + "'");
		threadPool.execute(task);
	}//execute
	
	/**
	 * Tries to stop the thread pool and all its threads  
	 */
	public void shutdown() {
		if ( threadPool == null || threadPool.isShutdown() ) {
			return;
		}
		
		threadPool.shutdown();
		threadPool = null;
	}//shutdown
	
	/**
	 * The callback is called, when there are no free threads to execute the given task  
	 */
	@Override
	public void rejectedExecution(Runnable r, ThreadPoolExecutor executor) {
		throw new RejectedExecutionException("Failed to execute the given task, all the worker threads are busy");
	}//rejectedExecution
	
	/**
	 * Initializes the thread pool
	 */
	private void initPool() {
		threadPool = new ThreadPoolExecutor(
				THREAD_CORE_NUM,
				THREAD_MAX_NUM,
				THREAD_TTL,
				TTL_TIME_UNIT,
				new SynchronousQueue<Runnable>(true) // The fair order FIFO
		);
		threadPool.setRejectedExecutionHandler(this);
	}//initPool
}

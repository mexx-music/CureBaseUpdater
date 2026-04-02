package com.example.curebaseupdater

import android.Manifest
import android.bluetooth.BluetoothManager
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.Button
import android.widget.ProgressBar
import android.widget.ScrollView
import android.widget.TextView
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch

private const val TAG = "CURE_UPDATER"

class MainActivity : AppCompatActivity() {
    private lateinit var btnConnect: Button
    private lateinit var btnUnlock: Button
    private lateinit var btnStartOta: Button
    private lateinit var btnShare: Button
    private lateinit var prog: ProgressBar
    private lateinit var tvLog: TextView
    private lateinit var scroll: ScrollView

    private val scope = CoroutineScope(Dispatchers.Main + Job())

    private lateinit var bleClient: BleUartClient
    private var protocol: CureProtocol? = null

    private var logBuilder = StringBuilder()

    // remember last selected device address
    private var connectedAddress: String? = null
    private var connectedName: String? = null

    // --- Toggle: post-unlock sanity check (kept simple; you can wire to UI later) ---
    private val POST_UNLOCK_SANITY_CHECK: Boolean by lazy {
        try {
            val cls = Class.forName("com.example.curebaseupdater.BuildConfig")
            val fld = cls.getField("DEBUG")
            fld.getBoolean(null)
        } catch (_: Throwable) {
            false
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        btnConnect = findViewById(R.id.btnConnect)
        btnUnlock = findViewById(R.id.btnUnlock)
        btnStartOta = findViewById(R.id.btnStartOta)
        btnShare = findViewById(R.id.btnShare)
        val btnLocationSettings = findViewById<Button>(R.id.btnLocationSettings)
        prog = findViewById(R.id.prog)
        tvLog = findViewById(R.id.tvLog)
        scroll = (tvLog.parent as? ScrollView) ?: ScrollView(this)

        btnLocationSettings.setOnClickListener {
            startActivity(Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS))
        }

        bleClient = BleUartClient(this)

        // listeners
        bleClient.addOnLineListener { line -> appendLog("RX: $line") }
        bleClient.addOnStateListener { st -> appendLog("STATE: $st") }

        // optional file picker
        registerForActivityResult(ActivityResultContracts.GetContent()) { uri: Uri? ->
            uri?.let { startOtaWithUri(it) }
        }

        btnConnect.setOnClickListener { onConnectClicked() }
        btnUnlock.setOnClickListener { onUnlockClicked() }
        btnStartOta.setOnClickListener { onStartOtaClicked() }
        // Long-press Start OTA -> trigger OTA keypair probe (safe fallback, no new button required)
        btnStartOta.setOnLongClickListener {
            onProbeRequested()
            true
        }
        btnShare.setOnClickListener { shareLog() }
    }

    private fun appendLog(s: String) {
        Log.d(TAG, s)
        runOnUiThread {
            logBuilder.append(s).append('\n')
            tvLog.text = logBuilder.toString()
            scroll.post { scroll.fullScroll(View.FOCUS_DOWN) }
        }
    }

    private fun checkPermissions(): Boolean {
        val perms = mutableListOf<String>()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            perms += Manifest.permission.BLUETOOTH_SCAN
            perms += Manifest.permission.BLUETOOTH_CONNECT
        } else {
            perms += Manifest.permission.ACCESS_FINE_LOCATION
            perms += Manifest.permission.ACCESS_COARSE_LOCATION
        }

        val notGranted = perms.filter {
            ContextCompat.checkSelfPermission(this, it) != PackageManager.PERMISSION_GRANTED
        }
        Log.d(TAG, "PERM check sdk=${Build.VERSION.SDK_INT} notGranted=$notGranted")
        if (notGranted.isNotEmpty()) {
            ActivityCompat.requestPermissions(this, notGranted.toTypedArray(), 1001)
            return false
        }

        val lm = getSystemService(LOCATION_SERVICE) as? android.location.LocationManager
        val enabled =
            lm?.isProviderEnabled(android.location.LocationManager.GPS_PROVIDER) == true ||
                    lm?.isProviderEnabled(android.location.LocationManager.NETWORK_PROVIDER) == true

        Log.d(TAG, "Location service enabled=$enabled")
        if (!enabled) {
            appendLog("Location services seem OFF. Some devices require Location ON for BLE scanning. Please enable it in system settings.")
        }
        return true
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        val grantedMap = permissions.mapIndexed { idx, perm ->
            perm to (grantResults.getOrNull(idx) == PackageManager.PERMISSION_GRANTED)
        }.toMap()
        Log.d(TAG, "onRequestPermissionsResult requestCode=$requestCode results=$grantedMap")
        appendLog("Permissions result: $grantedMap")
        if (grantedMap.values.all { it }) appendLog("Permissions granted")
        else appendLog("Permissions denied: $grantedMap")
    }

    private fun onConnectClicked() {
        if (!checkPermissions()) {
            appendLog("Permissions requested")
            return
        }
        val btEnabled = (getSystemService(BluetoothManager::class.java)?.adapter?.isEnabled == true)
        Log.d(TAG, "BT enabled=$btEnabled")
        appendLog("Start scan (collecting devices)")

        scope.launch {
            val candidates = bleClient.scan(timeoutMs = 15_000)
            if (candidates.isEmpty()) {
                appendLog("No BLE devices found. Ensure Bluetooth+Location service ON, Cube advertising.")
                return@launch
            }

            val top = candidates.sortedByDescending { it.rssi }.take(10)
            val items = top.map { c ->
                "${c.name ?: "(no name)"}    rssi=${c.rssi}    ${c.address}"
            }.toTypedArray()

            runOnUiThread {
                val builder = androidx.appcompat.app.AlertDialog.Builder(this@MainActivity)
                builder.setTitle("Select device to connect")
                builder.setItems(items) { _, which ->
                    val sel = top[which]
                    connectedAddress = sel.address
                    connectedName = sel.name

                    appendLog("Connecting to ${sel.name ?: "(no name)"} ${sel.address}")
                    scope.launch {
                        val ok = bleClient.connect(sel.address, 15_000)
                        appendLog("connect/probe ok=$ok")
                        if (ok) {
                            protocol = CureProtocol(bleClient)
                            appendLog("Protocol ready")
                        } else {
                            protocol = null
                        }
                    }
                }
                builder.setNegativeButton("Cancel", null)
                builder.show()
            }
        }
    }

    private suspend fun postUnlockSanityCheck(): Boolean {
        // Keep it minimal: these are small ASCII commands, handled by BleUartClient.writeOnce() path.
        // If your firmware doesn't support them, just set POST_UNLOCK_SANITY_CHECK=false.
        appendLog("Post-unlock sanity check: gethardware")
        val hwOk = bleClient.writeOnce("gethardware\r\n".toByteArray(Charsets.ISO_8859_1), timeoutMs = 6000L)
        appendLog("gethardware ok=$hwOk")

        appendLog("Post-unlock sanity check: getbuild")
        val buildOk = bleClient.writeOnce("getbuild\r\n".toByteArray(Charsets.ISO_8859_1), timeoutMs = 6000L)
        appendLog("getbuild ok=$buildOk")

        return hwOk && buildOk
    }

    private fun onUnlockClicked() {
        if (!checkPermissions()) {
            appendLog("Permissions requested")
            return
        }

        val addr = connectedAddress
        if (addr.isNullOrBlank()) {
            appendLog("No device address known. Please Connect/select device first.")
            return
        }

        btnUnlock.isEnabled = false
        btnConnect.isEnabled = false
        btnStartOta.isEnabled = false

        scope.launch {
            try {
                appendLog("Unlock (single-conn): starting... addr=$addr")

                val ok = bleClient.unlockSingleConnection(addr, connectTimeoutMs = 15000L)
                appendLog("Unlock (single-conn): ok=$ok")

                if (ok) {
                    appendLog("Reconnecting after unlock...")
                    val ok2 = bleClient.connect(addr, 15_000)
                    appendLog("reconnect/probe ok=$ok2")
                    if (ok2) {
                        protocol = CureProtocol(bleClient)
                        appendLog("Protocol ready (post-unlock)")

                        if (POST_UNLOCK_SANITY_CHECK) {
                            val sanityOk = postUnlockSanityCheck()
                            appendLog("Post-unlock sanity overall ok=$sanityOk")
                            if (!sanityOk) {
                                appendLog("WARNING: post-unlock sanity failed. OTA may be unstable; consider reconnect or retry.")
                            }
                        }
                    } else {
                        protocol = null
                    }
                }
            } catch (e: Exception) {
                appendLog("Unlock failed: ${e.message}")
            } finally {
                btnUnlock.isEnabled = true
                btnConnect.isEnabled = true
                btnStartOta.isEnabled = true
            }
        }
    }

    private fun onStartOtaClicked() {
        val p = protocol
        if (p == null) {
            appendLog("Not connected")
            return
        }
        scope.launch {
            appendLog("Starting OTA from assets/firmware/update.bin")
            val sender = OtaSender(p, this@MainActivity)
            prog.progress = 0
            val ok = sender.sendFromAssets("firmware/update.bin") { sent, total ->
                val pr = if (total > 0) (sent * 100 / total) else 0
                runOnUiThread { prog.progress = pr }
            }
            appendLog("OTA finished -> $ok")
        }
    }

    private fun startOtaWithUri(uri: Uri) {
        val p = protocol
        if (p == null) {
            appendLog("Not connected")
            return
        }
        scope.launch {
            appendLog("Starting OTA from URI: $uri")
            val sender = OtaSender(p, this@MainActivity)
            prog.progress = 0
            val ok = sender.sendFromUri(uri) { sent, total ->
                val pr = if (total > 0) (sent * 100 / total) else 0
                runOnUiThread { prog.progress = pr }
            }
            appendLog("OTA finished -> $ok")
        }
    }

    // Minimal guarded probe trigger. Keeps the change local to the UI handler.
    // Logs requested states and only starts probing when the protocol/client appears ready.
    private fun onProbeRequested() {
        appendLog("Probe requested")

        // Determine readiness: protocol != null is used elsewhere as "ready" (see connect flow)
        val clientReady = protocol != null
        appendLog("Probe starting: client ready=$clientReady")

        if (!clientReady) {
            appendLog("Probe blocked: not connected/ready")
            return
        }

        // Launch probe in background
        scope.launch {
            try {
                appendLog("probeKeypairs: starting")
                val sender = OtaSender(protocol!!, this@MainActivity)
                val found = sender.probeKeypairs()
                appendLog("probeKeypairs result: ${'$'}found")
            } catch (e: Exception) {
                appendLog("probeKeypairs exception: ${'$'}{e.message}")
            }
        }
    }

    private fun shareLog() {
        val i = Intent(Intent.ACTION_SEND)
        i.type = "text/plain"
        i.putExtra(Intent.EXTRA_TEXT, logBuilder.toString())
        startActivity(Intent.createChooser(i, "Share log"))
    }
}
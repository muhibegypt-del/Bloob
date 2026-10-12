// Decompile selected Elements of A-to-LG playback adapter functions.
// @category Elements of A

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class ExportPlaybackDecomp extends GhidraScript {
    private static final String[] ADDRESSES = {
        "000cb780", // tryPlay
        "000cb8c4", // setVideoWindow
        "000cc6e8", // handleLgEvent
        "000ccfa0", // prepare
        "000cd51c", // stop
        "000cdb48", // internalSubmitSample
        "000ce1f4"  // submitSample
    };

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) {
            throw new IllegalArgumentException("Expected one output path");
        }
        Path output = Path.of(args[0]);
        DecompInterface decompiler = new DecompInterface();
        decompiler.openProgram(currentProgram);
        try (BufferedWriter writer = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            for (String hex : ADDRESSES) {
                if (monitor.isCancelled()) break;
                Address address = currentProgram.getAddressFactory().getAddress(hex);
                Function function = currentProgram.getFunctionManager().getFunctionAt(address);
                if (function == null) {
                    writer.write("\n=== " + hex + " : no function ===\n");
                    continue;
                }
                writer.write("\n=== " + hex + " " + function.getName(true)
                    + " bytes=" + function.getBody().getNumAddresses() + " ===\n");
                DecompileResults result = decompiler.decompileFunction(function, 90, monitor);
                if (!result.decompileCompleted() || result.getDecompiledFunction() == null) {
                    writer.write("DECOMPILATION FAILED: " + result.getErrorMessage() + "\n");
                    continue;
                }
                String code = result.getDecompiledFunction().getC();
                for (String line : code.split("\n", -1)) {
                    String lower = line.toLowerCase();
                    if (lower.contains("drm") || lower.contains("license")
                            || lower.contains("decrypt") || lower.contains("crypto")) {
                        writer.write("[excluded protected-content line]\n");
                    } else {
                        writer.write(line + "\n");
                    }
                }
            }
        } finally {
            decompiler.dispose();
        }
        println("Wrote selected playback functions to " + output);
    }
}

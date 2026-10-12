// Enumerate playback-related native functions without exporting unrelated code.
// @category Elements of A

import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import java.io.BufferedWriter;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

public class ListMediaFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length != 1) {
            throw new IllegalArgumentException("Expected one output TSV path");
        }
        Path output = Path.of(args[0]);
        int count = 0;
        try (BufferedWriter writer = Files.newBufferedWriter(output, StandardCharsets.UTF_8)) {
            writer.write("address\tname\tbytes\n");
            FunctionIterator functions = currentProgram.getFunctionManager().getFunctions(true);
            while (functions.hasNext() && !monitor.isCancelled()) {
                Function function = functions.next();
                String name = function.getName(true);
                String lower = name.toLowerCase();
                boolean relevant = name.contains("LgEsPlayerPalImpl")
                    || name.contains("Media::PAL::EsPlayer")
                    || name.contains("Media::MsePlayer")
                    || name.contains("Media::SourceBuffer")
                    || name.contains("Media::Source")
                    || name.contains("Media::TrackPump")
                    || name.contains("Media::TrackBuffer")
                    || name.contains("DrawingContext::drawVideo");
                boolean excluded = lower.contains("drm") || lower.contains("license")
                    || lower.contains("decrypt") || lower.contains("crypto");
                if (relevant && !excluded) {
                    writer.write(function.getEntryPoint() + "\t" + name.replace('\t', ' ')
                        + "\t" + function.getBody().getNumAddresses() + "\n");
                    count++;
                }
            }
        }
        println("Wrote " + count + " relevant functions to " + output);
    }
}

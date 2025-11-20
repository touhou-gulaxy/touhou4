import kotlin.io.path.*
import kotlin.system.exitProcess

val locRoot = Path("/home/koishi/IdeaProjects/touhou4/localisation")
val languageDirs = arrayOf("braz_por", "german", "french", "spanish", "polish",
    "russian", "japanese", "korean").map { Path(locRoot.pathString, it) }
val languageEndfix = arrayOf("l_braz_por", "l_german", "l_french", "l_spanish",
    "l_polish", "l_russian", "l_japanese", "l_korean")
val oriLocfileNames = Path(locRoot.pathString, "simp_chinese")
    .listDirectoryEntries().filter { it.isRegularFile() }.map { it.name.removeSuffix("_l_simp_chinese.yml") }
oriLocfileNames.forEach { println(it) }
val engLocfiles = oriLocfileNames
    .map { Path(locRoot.pathString, "english", "${it}_l_english.yml") }

languageDirs.filter { !it.exists() }
    .forEach {
        println(it)
        it.createDirectories()
    }
// exitProcess(0)
languageDirs.filter { it.exists() }
    .forEachIndexed { index, dir ->
        oriLocfileNames.map { it to Path(dir.pathString, "${it}_${languageEndfix[index]}.yml") }
            .forEach {
                val nameIndex = oriLocfileNames.indexOf(it.first)
                if (nameIndex != -1) {
                    if (!it.second.exists())
                        it.second.createFile()
                    val lines = engLocfiles[nameIndex].readLines().toMutableList()
                    lines[0] = "${languageEndfix[index]}:"
                    val out = it.second.outputStream().buffered(1024)
//                    out.write(65279)
                    out.write(0xEF)
                    out.write(0xBB)
                    out.write(0xBF)
                    out.flush()
                    lines.forEach { line ->
                        out.write(line.toByteArray())
                        out.write(0x0A)
                        out.flush()
                    }
                    out.close()
                }
            }
    }

package org.opencv.test.features2d;

import org.opencv.test.OpenCVTestCase;
import org.opencv.test.OpenCVTestRunner;
import org.opencv.features2d.BRISK;

public class BRISKDescriptorExtractorTest extends OpenCVTestCase {

    BRISK extractor;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        extractor = BRISK.create(); // default (30,3,1)
    }

    public void testCreate() {
        assertNotNull(extractor);
    }

    public void testDetectListOfMatListOfListOfKeyPoint() {
        fail("Not yet implemented");
    }

    public void testDetectListOfMatListOfListOfKeyPointListOfMat() {
        fail("Not yet implemented");
    }

    public void testDetectMatListOfKeyPoint() {
        fail("Not yet implemented");
    }

    public void testDetectMatListOfKeyPointMat() {
        fail("Not yet implemented");
    }

    public void testEmpty() {
        fail("Not yet implemented");
    }

    public void testRead() {
        String filename = OpenCVTestRunner.getTempFileName("xml");
        writeFile(filename,
                "<?xml version=\"1.0\"?>\n<opencv_storage>\n<name>Feature2D.BRISK</name>\n<threshold>31</threshold>\n<octaves>4</octaves>\n<patternScale>1.1</patternScale>\n</opencv_storage>\n");

        extractor.read(filename);

        assertEquals(31, extractor.getThreshold());
        assertEquals(4, extractor.getOctaves());
        assertEquals((float) 1.1, extractor.getPatternScale());
    }

    public void testReadYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");
        writeFile(filename, "%YAML:1.0\n---\nname: \"Feature2D.BRISK\"\nthreshold: 31\noctaves: 4\npatternScale: 1.1\n");

        extractor.read(filename);

        assertEquals(31, extractor.getThreshold());
        assertEquals(4, extractor.getOctaves());
        assertEquals((float) 1.1, extractor.getPatternScale());
    }

    public void testWrite() {
        String filename = OpenCVTestRunner.getTempFileName("xml");

        extractor.write(filename);

        String truth = "<?xml version=\"1.0\"?>\n<opencv_storage>\n<name>Feature2D.BRISK</name>\n<threshold>30</threshold>\n<octaves>3</octaves>\n<patternScale>1.</patternScale>\n</opencv_storage>\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

    public void testWriteYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");

        extractor.write(filename);

        String truth = "%YAML:1.0\n---\nname: \"Feature2D.BRISK\"\nthreshold: 30\noctaves: 3\npatternScale: 1.\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

}

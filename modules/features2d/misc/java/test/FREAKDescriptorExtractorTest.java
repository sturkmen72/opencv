package org.opencv.test.features2d;

import org.opencv.test.OpenCVTestCase;
import org.opencv.test.OpenCVTestRunner;
import org.opencv.xfeatures2d.FREAK;

public class FREAKDescriptorExtractorTest extends OpenCVTestCase {

    FREAK extractor;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        extractor = FREAK.create(); // default (true,true,22,4)
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
        writeFile(filename, "<?xml version=\"1.0\"?>\n<opencv_storage>\n<name>Feature2D.FREAK</name>\n<orientationNormalized>0</orientationNormalized>\n<scaleNormalized>0</scaleNormalized>\n<patternScale>23.</patternScale>\n<nOctaves>5</nOctaves>\n</opencv_storage>\n");

        extractor.read(filename);

        assertEquals(false, extractor.getOrientationNormalized());
        assertEquals(false, extractor.getScaleNormalized());
        assertEquals((float) 23.0, extractor.getPatternScale());
        assertEquals(5, extractor.getNOctaves());
    }

    public void testReadYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");
        writeFile(filename, "%YAML:1.0\n---\nname: \"Feature2D.FREAK\"\norientationNormalized: 0\nscaleNormalized: 0\npatternScale: 23.\nnOctaves: 5\n");

        extractor.read(filename);

        assertEquals(false, extractor.getOrientationNormalized());
        assertEquals(false, extractor.getScaleNormalized());
        assertEquals((float) 23.0, extractor.getPatternScale());
        assertEquals(5, extractor.getNOctaves());
    }

    public void testWrite() {
        String filename = OpenCVTestRunner.getTempFileName("xml");

        extractor.write(filename);

        String truth = "<?xml version=\"1.0\"?>\n<opencv_storage>\n<name>Feature2D.FREAK</name>\n<orientationNormalized>1</orientationNormalized>\n<scaleNormalized>1</scaleNormalized>\n<patternScale>22.</patternScale>\n<nOctaves>4</nOctaves>\n</opencv_storage>\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

    public void testWriteYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");

        extractor.write(filename);

        String truth = "%YAML:1.0\n---\nname: \"Feature2D.FREAK\"\norientationNormalized: 1\nscaleNormalized: 1\npatternScale: 22.\nnOctaves: 4\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

}

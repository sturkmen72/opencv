package org.opencv.test.features2d;

import org.opencv.test.OpenCVTestCase;
import org.opencv.test.OpenCVTestRunner;
import org.opencv.xfeatures2d.LATCH;

public class LATCHDescriptorExtractorTest extends OpenCVTestCase {

    LATCH extractor;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        extractor = LATCH.create(); // default (32,true,3,2.0)
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
        writeFile(filename, "<?xml version=\"1.0\"?>\n<opencv_storage>\n<name>Feature2D.LATCH</name>\n<descriptorSize>64</descriptorSize>\n<rotationInvariance>0</rotationInvariance>\n<half_ssd_size>5</half_ssd_size>\n<sigma>3.</sigma>\n</opencv_storage>\n");

        extractor.read(filename);

        assertEquals(64, extractor.getBytes());
        assertEquals(false, extractor.getRotationInvariance());
        assertEquals(5, extractor.getHalfSSDsize());
        assertEquals(3.0, extractor.getSigma());
    }

    public void testReadYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");
        writeFile(filename, "%YAML:1.0\n---\nname: \"Feature2D.LATCH\"\ndescriptorSize: 64\nrotationInvariance: 0\nhalf_ssd_size: 5\nsigma: 3.\n");

        extractor.read(filename);

        assertEquals(64, extractor.getBytes());
        assertEquals(false, extractor.getRotationInvariance());
        assertEquals(5, extractor.getHalfSSDsize());
        assertEquals(3.0, extractor.getSigma());
    }

    public void testWrite() {
        String filename = OpenCVTestRunner.getTempFileName("xml");

        extractor.write(filename);

        String truth = "<?xml version=\"1.0\"?>\n<opencv_storage>\n<name>Feature2D.LATCH</name>\n<descriptorSize>32</descriptorSize>\n<rotationInvariance>1</rotationInvariance>\n<half_ssd_size>3</half_ssd_size>\n<sigma>2.</sigma>\n</opencv_storage>\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

    public void testWriteYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");

        extractor.write(filename);

        String truth = "%YAML:1.0\n---\nname: \"Feature2D.LATCH\"\ndescriptorSize: 32\nrotationInvariance: 1\nhalf_ssd_size: 3\nsigma: 2.\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

}
